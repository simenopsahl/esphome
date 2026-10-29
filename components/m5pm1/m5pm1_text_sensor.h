#pragma once

#include "esphome/core/defines.h"
#ifdef USE_TEXT_SENSOR

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "m5pm1.h"

namespace esphome {
namespace m5pm1 {

enum M5PM1TextSensorType : uint8_t {
  TEXT_POWER_SOURCE,
  TEXT_WAKE_SOURCE,
};

class M5PM1TextSensor : public text_sensor::TextSensor, public PollingComponent {
 public:
  void set_parent(M5PM1Component *parent) { this->parent_ = parent; }
  void set_type(M5PM1TextSensorType type) { this->type_ = type; }

  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  M5PM1Component *parent_;
  M5PM1TextSensorType type_;
};

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_TEXT_SENSOR
