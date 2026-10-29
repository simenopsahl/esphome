#include "esphome/core/defines.h"
#ifdef USE_LIGHT

#include "m5pm1_light.h"
#include "esphome/core/log.h"

namespace esphome {
namespace m5pm1 {

static const char *const TAG = "m5pm1.light";

void M5PM1Light::setup() {
  this->buf_ = new uint8_t[this->num_leds_ * 3]();
  this->effect_data_ = new uint8_t[this->num_leds_]();
  this->parent_->set_led_count((uint8_t) this->num_leds_);
}

light::ESPColorView M5PM1Light::get_view_internal(int32_t index) const {
  size_t pos = index * 3;
  return light::ESPColorView(this->buf_ + pos + 0, this->buf_ + pos + 1, this->buf_ + pos + 2, nullptr,
                             this->effect_data_ + index, &this->correction_);
}

void M5PM1Light::write_state(light::LightState *state) {
  for (uint16_t i = 0; i < this->num_leds_; i++) {
    size_t pos = i * 3;
    this->parent_->set_led_color((uint8_t) i, this->buf_[pos + 0], this->buf_[pos + 1], this->buf_[pos + 2]);
  }
  this->parent_->refresh_leds();
}

void M5PM1Light::dump_config() { ESP_LOGCONFIG(TAG, "M5PM1 NeoPixel light: %u LEDs", this->num_leds_); }

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_LIGHT
