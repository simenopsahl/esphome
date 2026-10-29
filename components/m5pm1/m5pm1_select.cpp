#include "esphome/core/defines.h"
#ifdef USE_SELECT

#include "m5pm1_select.h"
#include "esphome/core/log.h"

namespace esphome {
namespace m5pm1 {

static const char *const TAG = "m5pm1.select";

void M5PM1I2CSpeedSelect::control(const std::string &value) {
  bool fast = (value == "400kHz");
  if (this->parent_->switch_i2c_speed_400k(fast))
    this->publish_state(value);
  else
    ESP_LOGW(TAG, "'%s': write failed", this->get_name().c_str());
}

void M5PM1I2CSpeedSelect::dump_config() { LOG_SELECT("", "M5PM1 I2C Speed", this); }

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_SELECT
