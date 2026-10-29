#pragma once

#include "esphome/core/defines.h"
#ifdef USE_SELECT

#include "esphome/core/component.h"
#include "esphome/components/select/select.h"
#include "m5pm1.h"

namespace esphome {
namespace m5pm1 {

class M5PM1I2CSpeedSelect : public select::Select, public Component {
 public:
  void set_parent(M5PM1Component *parent) { this->parent_ = parent; }
  void dump_config() override;

 protected:
  void control(const std::string &value) override;
  M5PM1Component *parent_;
};

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_SELECT
