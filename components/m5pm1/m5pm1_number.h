#pragma once

#include "esphome/core/defines.h"
#ifdef USE_NUMBER

#include "esphome/core/component.h"
#include "esphome/components/number/number.h"
#include "m5pm1.h"

namespace esphome {
namespace m5pm1 {

enum M5PM1NumberType : uint8_t {
  NUMBER_BATTERY_LVP,
  NUMBER_WATCHDOG_TIMEOUT,
  NUMBER_PWM_FREQUENCY,
};

class M5PM1Number : public number::Number, public Component {
 public:
  void set_parent(M5PM1Component *parent) { this->parent_ = parent; }
  void set_type(M5PM1NumberType type) { this->type_ = type; }

  void dump_config() override;

 protected:
  void control(float value) override;
  M5PM1Component *parent_;
  M5PM1NumberType type_;
};

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_NUMBER
