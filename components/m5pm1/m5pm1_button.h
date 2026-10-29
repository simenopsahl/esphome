#pragma once

#include "esphome/core/defines.h"
#ifdef USE_BUTTON

#include "esphome/core/component.h"
#include "esphome/components/button/button.h"
#include "m5pm1.h"

namespace esphome {
namespace m5pm1 {

enum M5PM1ButtonType : uint8_t {
  BUTTON_SHUTDOWN,
  BUTTON_REBOOT,
  BUTTON_FEED_WATCHDOG,
  BUTTON_REFRESH_LEDS,
  BUTTON_ENTER_DOWNLOAD_MODE,
  BUTTON_TIMER_CLEAR,
};

class M5PM1Button : public button::Button, public Component {
 public:
  void set_parent(M5PM1Component *parent) { this->parent_ = parent; }
  void set_type(M5PM1ButtonType type) { this->type_ = type; }

  void dump_config() override;

 protected:
  void press_action() override;
  M5PM1Component *parent_;
  M5PM1ButtonType type_;
};

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_BUTTON
