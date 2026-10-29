#include "esphome/core/defines.h"
#ifdef USE_SWITCH

#include "m5pm1_switch.h"
#include "esphome/core/log.h"

namespace esphome {
namespace m5pm1 {

static const char *const TAG = "m5pm1.switch";

void M5PM1Switch::setup() {
  // Publish the current hardware state at boot so the entity reflects reality.
  uint8_t reg = 0;
  bool state = false;
  bool ok = false;
  switch (this->type_) {
    case SWITCH_CHARGE:
      ok = this->parent_->get_power_config(&reg);
      state = reg & PWR_CFG_CHG_EN;
      break;
    case SWITCH_DCDC:
      ok = this->parent_->get_power_config(&reg);
      state = reg & PWR_CFG_DCDC_EN;
      break;
    case SWITCH_LDO:
      ok = this->parent_->get_power_config(&reg);
      state = reg & PWR_CFG_LDO_EN;
      break;
    case SWITCH_BOOST:
      ok = this->parent_->get_power_config(&reg);
      state = reg & PWR_CFG_BOOST_EN;
      break;
    case SWITCH_LED_LEVEL:
      ok = this->parent_->get_power_config(&reg);
      state = reg & PWR_CFG_LED_CTRL;
      break;
    case SWITCH_LDO_HOLD:
      ok = this->parent_->get_hold_config(&reg);
      state = reg & HOLD_LDO;
      break;
    case SWITCH_BOOST_HOLD:
      ok = this->parent_->get_hold_config(&reg);
      state = reg & HOLD_BOOST;
      break;
    case SWITCH_GPIO_HOLD:
      ok = this->parent_->get_hold_config(&reg);
      state = reg & (1 << this->pin_);
      break;
  }
  if (ok)
    this->publish_state(state);
}

void M5PM1Switch::write_state(bool state) {
  bool ok = false;
  switch (this->type_) {
    case SWITCH_CHARGE:
      ok = this->parent_->set_charge_enable(state);
      break;
    case SWITCH_DCDC:
      ok = this->parent_->set_dcdc_enable(state);
      break;
    case SWITCH_LDO:
      ok = this->parent_->set_ldo_enable(state);
      break;
    case SWITCH_BOOST:
      ok = this->parent_->set_boost_enable(state);
      break;
    case SWITCH_LED_LEVEL:
      ok = this->parent_->set_led_en_level(state);
      break;
    case SWITCH_LDO_HOLD:
      ok = this->parent_->set_ldo_hold(state);
      break;
    case SWITCH_BOOST_HOLD:
      ok = this->parent_->set_boost_hold(state);
      break;
    case SWITCH_GPIO_HOLD:
      ok = this->parent_->set_gpio_hold(this->pin_, state);
      break;
  }
  if (ok)
    this->publish_state(state);
  else
    ESP_LOGW(TAG, "'%s': write failed", this->get_name().c_str());
}

void M5PM1Switch::dump_config() { LOG_SWITCH("", "M5PM1 Switch", this); }

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_SWITCH
