#include "esphome/core/defines.h"
#ifdef USE_BINARY_SENSOR

#include "m5pm1_binary_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace m5pm1 {

static const char *const TAG = "m5pm1.binary_sensor";

void M5PM1BinarySensor::update() {
  if (this->type_ == BINARY_BUTTON) {
    bool pressed = false;
    if (this->parent_->btn_get_state(&pressed))
      this->publish_state(pressed);
  } else {  // BINARY_EXTERNAL_POWER
    PowerSource src;
    if (this->parent_->get_power_source(&src))
      this->publish_state((src & (PWR_SRC_5VIN | PWR_SRC_5VINOUT)) != 0);
  }
}

void M5PM1BinarySensor::dump_config() { LOG_BINARY_SENSOR("", "M5PM1 Binary Sensor", this); }

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_BINARY_SENSOR
