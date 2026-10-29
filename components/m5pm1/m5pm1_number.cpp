#include "esphome/core/defines.h"
#ifdef USE_NUMBER

#include "m5pm1_number.h"
#include "esphome/core/log.h"

namespace esphome {
namespace m5pm1 {

static const char *const TAG = "m5pm1.number";

void M5PM1Number::control(float value) {
  bool ok = false;
  switch (this->type_) {
    case NUMBER_BATTERY_LVP:
      ok = this->parent_->set_battery_lvp((uint16_t) value);
      break;
    case NUMBER_WATCHDOG_TIMEOUT:
      ok = this->parent_->wdt_set((uint8_t) value);
      break;
    case NUMBER_PWM_FREQUENCY:
      ok = this->parent_->set_pwm_frequency((uint16_t) value);
      break;
  }
  if (ok)
    this->publish_state(value);
  else
    ESP_LOGW(TAG, "'%s': write failed", this->get_name().c_str());
}

void M5PM1Number::dump_config() { LOG_NUMBER("", "M5PM1 Number", this); }

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_NUMBER
