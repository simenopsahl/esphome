#pragma once

#include "esphome/core/hal.h"
#include "m5pm1.h"

namespace esphome {
namespace m5pm1 {

class M5PM1GPIOPin : public GPIOPin {
 public:
  void setup() override;
  void pin_mode(gpio::Flags flags) override;
  bool digital_read() override;
  void digital_write(bool value) override;
  size_t dump_summary(char *buffer, size_t len) const override;
  gpio::Flags get_flags() const override { return this->flags_; }

  void set_parent(M5PM1Component *parent) { this->parent_ = parent; }
  void set_pin(uint8_t pin) { this->pin_ = pin; }
  void set_inverted(bool inverted) { this->inverted_ = inverted; }
  void set_flags(gpio::Flags flags) { this->flags_ = flags; }

 protected:
  M5PM1Component *parent_;
  uint8_t pin_;
  bool inverted_{false};
  gpio::Flags flags_{gpio::FLAG_NONE};
};

}  // namespace m5pm1
}  // namespace esphome
