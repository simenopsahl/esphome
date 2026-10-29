#include "esphome/core/defines.h"
#ifdef USE_OUTPUT

#include "m5pm1_output.h"
#include "esphome/core/log.h"

namespace esphome {
namespace m5pm1 {

static const char *const TAG = "m5pm1.output";

void M5PM1PWMOutput::setup() {
  // Route the PWM pin to its special (OTHER) function: ch0 -> GPIO3, ch1 -> GPIO4.
  uint8_t pin = (this->channel_ == 0) ? 3 : 4;
  this->parent_->gpio_set_func(pin, GPIO_FUNC_OTHER);
  this->parent_->set_pwm_frequency(this->frequency_);
  this->parent_->set_pwm_duty12(this->channel_, 0, false, false);
}

void M5PM1PWMOutput::write_state(float state) {
  uint16_t duty12 = (uint16_t) (state * 4095.0f + 0.5f);
  if (duty12 > 4095)
    duty12 = 4095;
  bool enable = duty12 > 0;
  this->parent_->set_pwm_duty12(this->channel_, duty12, false, enable);
}

void M5PM1PWMOutput::dump_config() {
  ESP_LOGCONFIG(TAG, "M5PM1 PWM Output: channel=%u (GPIO%u), frequency=%uHz", this->channel_,
                this->channel_ == 0 ? 3 : 4, this->frequency_);
}

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_OUTPUT
