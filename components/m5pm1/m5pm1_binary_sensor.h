#pragma once

#include "esphome/core/defines.h"
#ifdef USE_BINARY_SENSOR

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "m5pm1.h"

namespace esphome {
namespace m5pm1 {

enum M5PM1BinarySensorType : uint8_t {
  BINARY_BUTTON,
  BINARY_EXTERNAL_POWER,
};

class M5PM1BinarySensor : public binary_sensor::BinarySensor, public PollingComponent {
 public:
  void set_parent(M5PM1Component *parent) { this->parent_ = parent; }
  void set_type(M5PM1BinarySensorType type) { this->type_ = type; }

  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  M5PM1Component *parent_;
  M5PM1BinarySensorType type_;
};

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_BINARY_SENSOR
