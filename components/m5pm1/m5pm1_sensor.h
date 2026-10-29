#pragma once

#include "esphome/core/defines.h"
#ifdef USE_SENSOR

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "m5pm1.h"

namespace esphome {
namespace m5pm1 {

enum M5PM1SensorType : uint8_t {
  SENSOR_BATTERY_VOLTAGE,
  SENSOR_VIN_VOLTAGE,
  SENSOR_VINOUT_VOLTAGE,
  SENSOR_REFERENCE_VOLTAGE,
  SENSOR_TEMPERATURE,
  SENSOR_ADC1,
  SENSOR_ADC2,
  SENSOR_WATCHDOG_COUNT,
  SENSOR_PWM_FREQUENCY,
};

class M5PM1Sensor : public sensor::Sensor, public PollingComponent {
 public:
  void set_parent(M5PM1Component *parent) { this->parent_ = parent; }
  void set_type(M5PM1SensorType type) { this->type_ = type; }

  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  M5PM1Component *parent_;
  M5PM1SensorType type_;
};

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_SENSOR
