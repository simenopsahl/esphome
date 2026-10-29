#pragma once

#include "esphome/core/defines.h"
#ifdef USE_OUTPUT

#include "esphome/core/component.h"
#include "esphome/components/output/float_output.h"
#include "m5pm1.h"

namespace esphome {
namespace m5pm1 {

// PWM float output. channel 0 = GPIO3 (PWM0), channel 1 = GPIO4 (PWM1).
class M5PM1PWMOutput : public output::FloatOutput, public Component {
 public:
  void set_parent(M5PM1Component *parent) { this->parent_ = parent; }
  void set_channel(uint8_t channel) { this->channel_ = channel; }
  void set_frequency(uint16_t frequency) { this->frequency_ = frequency; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

 protected:
  void write_state(float state) override;
  M5PM1Component *parent_;
  uint8_t channel_{0};
  uint16_t frequency_{1000};
};

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_OUTPUT
