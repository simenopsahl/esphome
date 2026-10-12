// Vendored from agillis/esphome-modular-lvgl-buttons components/lvgl_screenshot (MIT).
// Local changes:
//   * composite_top_layer_(): the LVGL top layer (header/footer/boot/OTA overlays)
//     is alpha-blended over the active screen, so screenshots match the display.
//   * Host only: GET /page/next, /page/prev and /page/<index> drive LVGL page
//     navigation, so pages can be inspected without touching the SDL window.

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include "stb_image_write.h"

#include "lvgl_screenshot.h"

#include "esphome/core/log.h"
#include <algorithm>
#include <cstring>

#ifdef USE_ESP_IDF
#include "esp_heap_caps.h"
#endif

#ifdef USE_HOST
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#endif

namespace esphome {
namespace lvgl_screenshot {

static const char *const TAG = "lvgl_screenshot";

LvglScreenshot *LvglScreenshot::instance_ = nullptr;

// Context passed to stb's PNG write callback
struct PngWriteCtx {
  uint8_t *buf;
  size_t capacity;
  size_t size;
};

// ---------------------------------------------------------------------------
// png_write_cb_()  –  stb calls this repeatedly as it produces PNG data
// ---------------------------------------------------------------------------
void LvglScreenshot::png_write_cb_(void *ctx, void *data, int size) {
  auto *c = (PngWriteCtx *) ctx;
  if (size <= 0 || !data)
    return;
  size_t avail = c->capacity - c->size;
  size_t copy = std::min((size_t) size, avail);
  if (copy < (size_t) size) {
    ESP_LOGW(TAG, "PNG buffer full — truncating %d → %u bytes", size, (unsigned) copy);
  }
  memcpy(c->buf + c->size, data, copy);
  c->size += copy;
}

// ---------------------------------------------------------------------------
// composite_top_layer_()  –  alpha-blend lv_layer_top() over rgb_buf_
//
// lv_snapshot_take() on the active screen does not include the display layers.
// Overlays placed on the top layer (status bars, boot/OTA screens) would be
// missing, so render the top layer as ARGB8888 and blend it on top.
// ---------------------------------------------------------------------------
void LvglScreenshot::composite_top_layer_(uint32_t width, uint32_t height) {
  lv_obj_t *top = lv_layer_top();
  if (top == nullptr || lv_obj_get_child_count(top) == 0)
    return;

  lv_draw_buf_t *ov = lv_snapshot_take(top, LV_COLOR_FORMAT_ARGB8888);
  if (ov == nullptr) {
    ESP_LOGW(TAG, "Top layer snapshot failed — returning screen only");
    return;
  }

  uint32_t w = std::min<uint32_t>(width, ov->header.w);
  uint32_t h = std::min<uint32_t>(height, ov->header.h);
  uint32_t stride = ov->header.stride;
  const uint8_t *src = static_cast<const uint8_t *>(ov->data);

  for (uint32_t y = 0; y < h; y++) {
    const uint8_t *srow = src + (size_t) y * stride;
    uint8_t *drow = this->rgb_buf_ + (size_t) y * width * 3u;
    for (uint32_t x = 0; x < w; x++) {
      // LVGL 9 ARGB8888 memory order is B,G,R,A
      uint32_t a = srow[x * 4 + 3];
      if (a == 0)
        continue;
      uint8_t *d = drow + x * 3;
      if (a == 255) {
        d[0] = srow[x * 4 + 2];
        d[1] = srow[x * 4 + 1];
        d[2] = srow[x * 4 + 0];
      } else {
        uint32_t ia = 255 - a;
        d[0] = (uint8_t) ((srow[x * 4 + 2] * a + d[0] * ia) / 255);
        d[1] = (uint8_t) ((srow[x * 4 + 1] * a + d[1] * ia) / 255);
        d[2] = (uint8_t) ((srow[x * 4 + 0] * a + d[2] * ia) / 255);
      }
    }
  }

  lv_draw_buf_destroy(ov);
}

// ---------------------------------------------------------------------------
// do_capture_()  –  render the active screen to RGB888, then encode to PNG
// (shared across both platforms)
//
// LVGL 9 removed the v8 `disp->driver->draw_buf` framebuffer access and its
// RGB565 `lv_color_t` bitfields. Instead we use lv_snapshot_take(), which
// re-renders the whole object tree into a fresh RGB888 buffer — independent of
// the display's own (often partial) draw buffer, so it always yields a
// complete, up-to-date frame. Requires LV_USE_SNAPSHOT (enabled via a build
// flag in __init__.py).
// ---------------------------------------------------------------------------
void LvglScreenshot::do_capture_() {
  lv_obj_t *screen = lv_screen_active();
  if (screen == nullptr) {
    ESP_LOGE(TAG, "No active LVGL screen");
    this->png_size_ = 0;
    return;
  }

  lv_draw_buf_t *snap = lv_snapshot_take(screen, LV_COLOR_FORMAT_RGB888);
  if (snap == nullptr) {
    ESP_LOGE(TAG, "lv_snapshot_take failed (out of memory?)");
    this->png_size_ = 0;
    return;
  }

  uint32_t width = snap->header.w;
  uint32_t height = snap->header.h;
  uint32_t stride = snap->header.stride;  // bytes per row (may be padded)
  const uint8_t *src = static_cast<const uint8_t *>(snap->data);

  // Guard against a snapshot larger than the buffers sized at setup().
  if (width * height * 3u > this->png_capacity_) {
    ESP_LOGE(TAG, "Snapshot %ux%u exceeds buffer capacity", (unsigned) width, (unsigned) height);
    lv_draw_buf_destroy(snap);
    this->png_size_ = 0;
    return;
  }

  // LVGL 9 stores RGB888 as B,G,R per pixel; stb/PNG wants R,G,B. Repack
  // row-by-row (honouring the buffer stride) into the contiguous rgb_buf_.
  for (uint32_t y = 0; y < height; y++) {
    const uint8_t *srow = src + (size_t) y * stride;
    uint8_t *drow = this->rgb_buf_ + (size_t) y * width * 3u;
    for (uint32_t x = 0; x < width; x++) {
      drow[x * 3 + 0] = srow[x * 3 + 2];  // R
      drow[x * 3 + 1] = srow[x * 3 + 1];  // G
      drow[x * 3 + 2] = srow[x * 3 + 0];  // B
    }
  }

  lv_draw_buf_destroy(snap);

  this->composite_top_layer_(width, height);

  // ------------------------------------------------------------------
  // Encode RGB888 → PNG via stb_image_write
  // ------------------------------------------------------------------
  PngWriteCtx ctx = {this->png_buf_, this->png_capacity_, 0};
  int png_stride = (int) (width * 3u);
  stbi_write_png_to_func(LvglScreenshot::png_write_cb_, &ctx, (int) width, (int) height, 3, this->rgb_buf_,
                         png_stride);

  this->png_size_ = ctx.size;
  ESP_LOGD(TAG, "Captured %ux%u PNG (%u bytes)", (unsigned) width, (unsigned) height, (unsigned) this->png_size_);
}

// ===========================================================================
// ESP-IDF implementation  (esp_http_server + FreeRTOS semaphores + PSRAM)
// ===========================================================================
#ifdef USE_ESP_IDF

void LvglScreenshot::setup() {
  instance_ = this;

  this->capture_requested_ = xSemaphoreCreateBinary();
  this->capture_done_ = xSemaphoreCreateBinary();

  if (!this->capture_requested_ || !this->capture_done_) {
    ESP_LOGE(TAG, "Failed to create semaphores");
    this->mark_failed();
    return;
  }

  lv_disp_t *disp = lv_disp_get_default();
  if (!disp) {
    ESP_LOGE(TAG, "No LVGL display found");
    this->mark_failed();
    return;
  }

  uint32_t width = (uint32_t) lv_disp_get_hor_res(disp);
  uint32_t height = (uint32_t) lv_disp_get_ver_res(disp);

  size_t rgb_size = width * height * 3u;
  this->rgb_buf_ = (uint8_t *) heap_caps_malloc(rgb_size, MALLOC_CAP_SPIRAM);
  if (!this->rgb_buf_) {
    ESP_LOGE(TAG, "Failed to allocate %u bytes for RGB buffer in PSRAM", (unsigned) rgb_size);
    this->mark_failed();
    return;
  }

  this->png_capacity_ = rgb_size;
  this->png_buf_ = (uint8_t *) heap_caps_malloc(this->png_capacity_, MALLOC_CAP_SPIRAM);
  if (!this->png_buf_) {
    ESP_LOGE(TAG, "Failed to allocate %u bytes for PNG buffer in PSRAM", (unsigned) this->png_capacity_);
    this->mark_failed();
    return;
  }

  this->png_size_ = 0;
  this->start_server_();
  ESP_LOGI(TAG, "LVGL screenshot server started — http://<device-ip>:%u/screenshot", this->port_);
}

void LvglScreenshot::start_server_() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = this->port_;
  cfg.stack_size = 8192;
  cfg.ctrl_port = (uint16_t) (this->port_ + 1u);

  if (httpd_start(&this->server_, &cfg) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start HTTP server on port %u", this->port_);
    this->server_ = nullptr;
    return;
  }

  httpd_uri_t uri = {
      .uri = "/screenshot",
      .method = HTTP_GET,
      .handler = LvglScreenshot::handle_screenshot_,
      .user_ctx = nullptr,
  };
  httpd_register_uri_handler(this->server_, &uri);
}

void LvglScreenshot::loop() {
  if (xSemaphoreTake(this->capture_requested_, 0) == pdTRUE) {
    this->do_capture_();
    xSemaphoreGive(this->capture_done_);
  }
}

esp_err_t LvglScreenshot::handle_screenshot_(httpd_req_t *req) {
  LvglScreenshot *self = instance_;
  if (!self || !self->png_buf_) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Component not ready");
    return ESP_FAIL;
  }

  if (self->in_progress_) {
    httpd_resp_set_status(req, "503 Service Unavailable");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "Capture in progress, try again");
    return ESP_OK;
  }
  self->in_progress_ = true;

  xSemaphoreGive(self->capture_requested_);

  if (xSemaphoreTake(self->capture_done_, pdMS_TO_TICKS(3000)) != pdTRUE) {
    self->in_progress_ = false;
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Capture timed out");
    return ESP_FAIL;
  }

  if (self->png_size_ == 0) {
    self->in_progress_ = false;
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Framebuffer unavailable");
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "image/png");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=\"screenshot.png\"");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store");

  const size_t CHUNK = 4096;
  size_t sent = 0;
  esp_err_t ret = ESP_OK;
  while (sent < self->png_size_) {
    size_t chunk_len = std::min(CHUNK, self->png_size_ - sent);
    ret = httpd_resp_send_chunk(req, (const char *) self->png_buf_ + sent, (ssize_t) chunk_len);
    if (ret != ESP_OK) {
      ESP_LOGW(TAG, "Failed to send chunk at offset %u", (unsigned) sent);
      break;
    }
    sent += chunk_len;
  }

  httpd_resp_send_chunk(req, nullptr, 0);
  self->in_progress_ = false;
  return ret;
}

#endif  // USE_ESP_IDF

// ===========================================================================
// Host / SDL implementation  (POSIX sockets + pthreads)
// ===========================================================================
#ifdef USE_HOST

void LvglScreenshot::setup() {
  instance_ = this;

  pthread_mutex_init(&this->capture_mutex_, nullptr);
  pthread_cond_init(&this->capture_request_cond_, nullptr);
  pthread_cond_init(&this->capture_done_cond_, nullptr);

  lv_disp_t *disp = lv_disp_get_default();
  if (!disp) {
    ESP_LOGE(TAG, "No LVGL display found");
    this->mark_failed();
    return;
  }

  uint32_t width = (uint32_t) lv_disp_get_hor_res(disp);
  uint32_t height = (uint32_t) lv_disp_get_ver_res(disp);

  size_t rgb_size = width * height * 3u;
  this->rgb_buf_ = (uint8_t *) malloc(rgb_size);
  if (!this->rgb_buf_) {
    ESP_LOGE(TAG, "Failed to allocate %u bytes for RGB buffer", (unsigned) rgb_size);
    this->mark_failed();
    return;
  }

  // PNG output is normally far smaller than raw RGB, but give headroom for
  // incompressible content (stb adds per-row filter bytes + zlib framing).
  this->png_capacity_ = rgb_size + height + 4096u;
  this->png_buf_ = (uint8_t *) malloc(this->png_capacity_);
  if (!this->png_buf_) {
    ESP_LOGE(TAG, "Failed to allocate %u bytes for PNG buffer", (unsigned) this->png_capacity_);
    this->mark_failed();
    return;
  }

  this->png_size_ = 0;
  this->start_server_();
  ESP_LOGI(TAG, "LVGL screenshot server started — http://localhost:%u/screenshot", this->port_);
}

void LvglScreenshot::start_server_() {
  this->server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (this->server_fd_ < 0) {
    ESP_LOGE(TAG, "Failed to create socket: %s", strerror(errno));
    return;
  }

  int opt = 1;
  setsockopt(this->server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr {};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(this->port_);

  if (bind(this->server_fd_, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    ESP_LOGE(TAG, "Failed to bind on port %u: %s", this->port_, strerror(errno));
    close(this->server_fd_);
    this->server_fd_ = -1;
    return;
  }

  if (listen(this->server_fd_, 2) < 0) {
    ESP_LOGE(TAG, "Failed to listen: %s", strerror(errno));
    close(this->server_fd_);
    this->server_fd_ = -1;
    return;
  }

  this->server_running_ = true;
  pthread_create(&this->server_thread_, nullptr, LvglScreenshot::server_thread_func_, this);
}

void *LvglScreenshot::server_thread_func_(void *arg) {
  auto *self = (LvglScreenshot *) arg;
  while (self->server_running_) {
    int client_fd = accept(self->server_fd_, nullptr, nullptr);
    if (client_fd < 0) {
      if (self->server_running_)
        ESP_LOGW(TAG, "accept() failed: %s", strerror(errno));
      continue;
    }
    // Set a receive timeout so read() doesn't block forever on
    // connections that never send data (e.g. mDNS probes)
    struct timeval tv = {2, 0};  // 2 second timeout
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    self->handle_client_(client_fd);
    close(client_fd);
  }
  return nullptr;
}

void LvglScreenshot::send_text_(int client_fd, const char *status, const char *body) {
  char resp[256];
  int len = snprintf(resp, sizeof(resp),
                     "HTTP/1.1 %s\r\nContent-Type: text/plain\r\nCache-Control: no-cache, no-store\r\n"
                     "Content-Length: %zu\r\n\r\n%s",
                     status, strlen(body), body);
  if (len > 0)
    write(client_fd, resp, std::min((size_t) len, sizeof(resp) - 1));
}

// Hand an action to the main loop (LVGL is not thread safe) and wait for it.
bool LvglScreenshot::run_on_main_loop_(Action action, int arg) {
  pthread_mutex_lock(&this->capture_mutex_);
  this->action_ = action;
  this->action_arg_ = arg;
  this->capture_requested_ = true;
  this->capture_done_ = false;
  pthread_cond_signal(&this->capture_request_cond_);

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 3;
  while (!this->capture_done_) {
    if (pthread_cond_timedwait(&this->capture_done_cond_, &this->capture_mutex_, &ts) != 0) {
      ESP_LOGW(TAG, "Timed out waiting for main loop");
      break;
    }
  }
  bool done = this->capture_done_.load();
  pthread_mutex_unlock(&this->capture_mutex_);
  return done;
}

void LvglScreenshot::handle_client_(int client_fd) {
  // Read the HTTP request
  char req_buf[512];
  ssize_t n = read(client_fd, req_buf, sizeof(req_buf) - 1);
  if (n <= 0)
    return;
  req_buf[n] = '\0';

  if (strncmp(req_buf, "GET ", 4) != 0) {
    this->send_text_(client_fd, "405 Method Not Allowed", "GET only\n");
    return;
  }
  const char *path = req_buf + 4;

  if (this->in_progress_) {
    this->send_text_(client_fd, "503 Service Unavailable", "Request in progress, retry.\n");
    return;
  }

  // --- Page navigation: /page/next, /page/prev, /page/<index> -------------
  if (strncmp(path, "/page/", 6) == 0) {
    const char *arg = path + 6;
    Action action;
    int index = 0;
    if (strncmp(arg, "next", 4) == 0) {
      action = ACTION_PAGE_NEXT;
    } else if (strncmp(arg, "prev", 4) == 0) {
      action = ACTION_PAGE_PREV;
    } else if (arg[0] >= '0' && arg[0] <= '9') {
      action = ACTION_PAGE_INDEX;
      index = atoi(arg);
    } else {
      this->send_text_(client_fd, "400 Bad Request", "Use /page/next, /page/prev or /page/<index>\n");
      return;
    }
    if (this->lvgl_ == nullptr) {
      this->send_text_(client_fd, "500 Internal Server Error", "No LVGL component\n");
      return;
    }
    this->in_progress_ = true;
    bool done = this->run_on_main_loop_(action, index);
    this->in_progress_ = false;
    this->send_text_(client_fd, done ? "200 OK" : "500 Internal Server Error", done ? "ok\n" : "timeout\n");
    return;
  }

  // --- Screenshot --------------------------------------------------------
  if (strncmp(path, "/screenshot", 11) != 0) {
    this->send_text_(client_fd, "404 Not Found", "Use /screenshot or /page/{next,prev,<index>}\n");
    return;
  }

  ESP_LOGD(TAG, "Screenshot request received");
  this->in_progress_ = true;
  bool done = this->run_on_main_loop_(ACTION_CAPTURE, 0);

  if (!done || this->png_size_ == 0) {
    this->send_text_(client_fd, "500 Internal Server Error", "capture failed\n");
    this->in_progress_ = false;
    return;
  }

  // Send HTTP response with PNG data
  char header[256];
  int hlen = snprintf(header, sizeof(header),
                      "HTTP/1.1 200 OK\r\n"
                      "Content-Type: image/png\r\n"
                      "Content-Disposition: inline; filename=\"screenshot.png\"\r\n"
                      "Cache-Control: no-cache, no-store\r\n"
                      "Content-Length: %zu\r\n"
                      "\r\n",
                      this->png_size_);
  write(client_fd, header, hlen);

  size_t sent = 0;
  while (sent < this->png_size_) {
    ssize_t w = write(client_fd, this->png_buf_ + sent, this->png_size_ - sent);
    if (w <= 0)
      break;
    sent += (size_t) w;
  }

  this->in_progress_ = false;
}

void LvglScreenshot::loop() {
  if (!this->capture_requested_.load())
    return;

  pthread_mutex_lock(&this->capture_mutex_);
  switch (this->action_.load()) {
    case ACTION_CAPTURE:
      this->do_capture_();
      break;
    case ACTION_PAGE_NEXT:
      this->lvgl_->show_next_page(LV_SCREEN_LOAD_ANIM_NONE, 0);
      break;
    case ACTION_PAGE_PREV:
      this->lvgl_->show_prev_page(LV_SCREEN_LOAD_ANIM_NONE, 0);
      break;
    case ACTION_PAGE_INDEX:
      this->lvgl_->show_page((size_t) this->action_arg_.load(), LV_SCREEN_LOAD_ANIM_NONE, 0);
      break;
    default:
      break;
  }
  this->action_ = ACTION_NONE;
  this->capture_requested_ = false;
  this->capture_done_ = true;
  pthread_cond_signal(&this->capture_done_cond_);
  pthread_mutex_unlock(&this->capture_mutex_);
}

#endif  // USE_HOST

}  // namespace lvgl_screenshot
}  // namespace esphome

