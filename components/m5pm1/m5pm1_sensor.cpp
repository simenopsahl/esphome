#include "esphome/core/defines.h"
#ifdef USE_SENSOR

#include "m5pm1_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace m5pm1 {

static const char *const TAG = "m5pm1.sensor";

void M5PM1Sensor::setup() {
  // ADC pins must be routed to their special function (OTHER) before sampling.
  if (this->type_ == SENSOR_ADC1)
    this->parent_->gpio_set_func(1, GPIO_FUNC_OTHER);
  else if (this->type_ == SENSOR_ADC2)
    this->parent_->gpio_set_func(2, GPIO_FUNC_OTHER);
}

void M5PM1Sensor::update() {
  bool ok = false;
  float value = NAN;
  uint16_t raw = 0;
  uint8_t raw8 = 0;

  switch (this->type_) {
    case SENSOR_BATTERY_VOLTAGE:
      ok = this->parent_->read_vbat(&raw);
      value = raw / 1000.0f;
      break;
    case SENSOR_VIN_VOLTAGE:
      ok = this->parent_->read_vin(&raw);
      value = raw / 1000.0f;
      break;
    case SENSOR_VINOUT_VOLTAGE:
      ok = this->parent_->read_5vinout(&raw);
      value = raw / 1000.0f;
      break;
    case SENSOR_REFERENCE_VOLTAGE:
      ok = this->parent_->read_vref(&raw);
      value = raw / 1000.0f;
      break;
    case SENSOR_TEMPERATURE:
      ok = this->parent_->read_temperature_deci(&raw);
      value = raw / 10.0f;
      break;
    case SENSOR_ADC1:
      ok = this->parent_->read_adc(ADC_CH_1, &raw);
      value = raw;
      break;
    case SENSOR_ADC2:
      ok = this->parent_->read_adc(ADC_CH_2, &raw);
      value = raw;
      break;
    case SENSOR_WATCHDOG_COUNT:
      ok = this->parent_->wdt_get_count(&raw8);
      value = raw8;
      break;
    case SENSOR_PWM_FREQUENCY:
      ok = this->parent_->get_pwm_frequency(&raw);
      value = raw;
      break;
  }

  if (ok) {
    this->publish_state(value);
  } else {
    ESP_LOGW(TAG, "'%s': read failed", this->get_name().c_str());
    this->publish_state(NAN);
  }
}

void M5PM1Sensor::dump_config() { LOG_SENSOR("", "M5PM1 Sensor", this); }

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_SENSOR
