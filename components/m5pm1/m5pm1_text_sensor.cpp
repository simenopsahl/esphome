#include "esphome/core/defines.h"
#ifdef USE_TEXT_SENSOR

#include "m5pm1_text_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace m5pm1 {

static const char *const TAG = "m5pm1.text_sensor";

static std::string power_source_to_string(PowerSource src) {
  if (src == 0)
    return "None";
  std::string result;
  if (src & PWR_SRC_5VIN)
    result += "5VIN";
  if (src & PWR_SRC_5VINOUT) {
    if (!result.empty())
      result += "+";
    result += "5VINOUT";
  }
  if (src & PWR_SRC_BAT) {
    if (!result.empty())
      result += "+";
    result += "Battery";
  }
  return result;
}

void M5PM1TextSensor::update() {
  if (this->type_ == TEXT_POWER_SOURCE) {
    PowerSource src;
    if (this->parent_->get_power_source(&src))
      this->publish_state(power_source_to_string(src));
  } else {  // TEXT_WAKE_SOURCE
    uint8_t flags = 0;
    if (!this->parent_->get_wake_source(&flags))
      return;
    std::string result;
    static const char *const NAMES[7] = {"timer",   "vin",     "power_button", "reset_button",
                                          "cmd_rst", "ext_gpio", "5vinout"};
    for (int i = 0; i < 7; i++) {
      if (flags & (1 << i)) {
        if (!result.empty())
          result += ",";
        result += NAMES[i];
      }
    }
    if (result.empty())
      result = "none";
    this->publish_state(result);
  }
}

void M5PM1TextSensor::dump_config() { LOG_TEXT_SENSOR("", "M5PM1 Text Sensor", this); }

}  // namespace m5pm1
}  // namespace esphome

#endif  // USE_TEXT_SENSOR
