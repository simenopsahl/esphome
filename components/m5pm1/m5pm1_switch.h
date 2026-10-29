#pragma once

#include "esphome/core/defines.h"
#ifdef USE_SWITCH

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "m5pm1.h"

namespace esphome {
namespace m5pm1 {

enum M5PM1SwitchType : uint8_t {
  SWITCH_CHARGE,
  SWITCH_DCDC,
  SWITCH_LDO,
  SWITCH_BOOST,
  SWITCH_LED_LEVEL,
  SWITCH_LDO_HOLD,
  SWITCH_BOOST_HOLD,
  SWITCH_GPIO_HOLD,
};

class M5PM1Switch : public switch_::Switch, public Component {
 public:
  void set_parent(M5PM1Component *parent) { this->parent_ = parent; }
  void set_type(M5PM1SwitchType type) { this->type_ = type; }
  void set_pin(uint8_t pin) { this->pin_ = pin; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void write_state(bool state) override;
  M5PM1Component *parent_;
  M5PM1SwitchType type_;
  uint8_t pin_{0};
};

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_SWITCH
