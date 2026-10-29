#include "esphome/core/defines.h"
#ifdef USE_BUTTON

#include "m5pm1_button.h"
#include "esphome/core/log.h"

namespace esphome {
namespace m5pm1 {

static const char *const TAG = "m5pm1.button";

void M5PM1Button::press_action() {
  switch (this->type_) {
    case BUTTON_SHUTDOWN:
      this->parent_->shutdown_pmic();
      break;
    case BUTTON_REBOOT:
      this->parent_->reboot_pmic();
      break;
    case BUTTON_FEED_WATCHDOG:
      this->parent_->wdt_feed();
      break;
    case BUTTON_REFRESH_LEDS:
      this->parent_->refresh_leds();
      break;
    case BUTTON_ENTER_DOWNLOAD_MODE:
      this->parent_->enter_download_mode();
      break;
    case BUTTON_TIMER_CLEAR:
      this->parent_->timer_clear();
      break;
  }
}

void M5PM1Button::dump_config() { LOG_BUTTON("", "M5PM1 Button", this); }

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_BUTTON
