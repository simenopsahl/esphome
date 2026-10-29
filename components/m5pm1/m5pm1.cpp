#include "m5pm1.h"
#include "esphome/core/log.h"

namespace esphome {
namespace m5pm1 {

static const char *const TAG = "m5pm1";

// ============================================================================
// Low-level register access
// ============================================================================

bool M5PM1Component::read_reg(uint8_t reg, uint8_t *value) {
  return this->read_register(reg, value, 1) == i2c::ERROR_OK;
}

bool M5PM1Component::write_reg(uint8_t reg, uint8_t value) {
  return this->write_register(reg, &value, 1) == i2c::ERROR_OK;
}

bool M5PM1Component::read_reg16(uint8_t reg, uint16_t *value) {
  uint8_t buf[2];
  if (this->read_register(reg, buf, 2) != i2c::ERROR_OK)
    return false;
  // Little-endian: low byte first
  *value = (uint16_t) buf[0] | ((uint16_t) buf[1] << 8);
  return true;
}

bool M5PM1Component::write_reg16(uint8_t reg, uint16_t value) {
  uint8_t buf[2] = {(uint8_t) (value & 0xFF), (uint8_t) ((value >> 8) & 0xFF)};
  return this->write_register(reg, buf, 2) == i2c::ERROR_OK;
}

bool M5PM1Component::read_bytes_reg(uint8_t reg, uint8_t *data, uint8_t len) {
  return this->read_register(reg, data, len) == i2c::ERROR_OK;
}

bool M5PM1Component::write_bytes_reg(uint8_t reg, const uint8_t *data, uint8_t len) {
  return this->write_register(reg, data, len) == i2c::ERROR_OK;
}

bool M5PM1Component::update_bits(uint8_t reg, uint8_t mask, uint8_t value) {
  uint8_t current;
  if (!this->read_reg(reg, &current))
    return false;
  current = (current & ~mask) | (value & mask);
  return this->write_reg(reg, current);
}

// ============================================================================
// Component lifecycle
// ============================================================================

void M5PM1Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up M5PM1...");
  uint8_t id = 0;
  if (!this->get_device_id(&id)) {
    ESP_LOGE(TAG, "Communication with M5PM1 failed");
    this->mark_failed();
    return;
  }
  ESP_LOGCONFIG(TAG, "  Device ID: 0x%02X", id);

  if (this->i2c_sleep_boot_ >= 0)
    this->set_i2c_sleep_time((uint8_t) this->i2c_sleep_boot_);
  if (this->battery_lvp_boot_ >= 0)
    this->set_battery_lvp((uint16_t) this->battery_lvp_boot_);
  if (this->charge_boot_ >= 0)
    this->set_charge_enable(this->charge_boot_ != 0);
  if (this->dcdc_boot_ >= 0)
    this->set_dcdc_enable(this->dcdc_boot_ != 0);
  if (this->ldo_boot_ >= 0)
    this->set_ldo_enable(this->ldo_boot_ != 0);
  if (this->boost_boot_ >= 0)
    this->set_boost_enable(this->boost_boot_ != 0);
  if (this->l3b_boot_ >= 0) {
    this->gpio_set_func(2, GPIO_FUNC_GPIO);
    this->gpio_set_drive(2, GPIO_DRIVE_PUSHPULL);
    this->gpio_set_mode(2, true);
    this->gpio_set_output(2, this->l3b_boot_ != 0);
  }
  if ((this->boost_boot_ > 0) || (this->l3b_boot_ > 0)) {
    // Allow external Grove / HAT sensors and rails to power up and stabilize
    delay(100);
  }
}

void M5PM1Component::dump_config() {
  ESP_LOGCONFIG(TAG, "M5PM1:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Communication failed!");
    return;
  }
  uint8_t hw = 0, sw = 0, model = 0;
  this->read_reg(REG_HW_REV, &hw);
  this->read_reg(REG_SW_REV, &sw);
  this->read_reg(REG_DEVICE_MODEL, &model);
  ESP_LOGCONFIG(TAG, "  Model: 0x%02X  HW rev: 0x%02X  FW rev: 0x%02X", model, hw, sw);
}

// ============================================================================
// ADC / temperature
// ============================================================================

bool M5PM1Component::read_adc(AdcChannel channel, uint16_t *value) {
  // Start conversion: [3:1] channel, [0] start
  uint8_t ctrl = (uint8_t) ((channel << 1) | 0x01);
  if (!this->write_reg(REG_ADC_CTRL, ctrl))
    return false;

  // Poll until the start bit clears (max ~500 ms)
  uint8_t reg = 0;
  int tries = 0;
  do {
    delay(10);
    if (!this->read_reg(REG_ADC_CTRL, &reg))
      return false;
    tries++;
  } while ((reg & 0x01) && tries < 50);

  if (reg & 0x01) {
    ESP_LOGW(TAG, "ADC conversion timeout on channel %d", channel);
    return false;
  }

  if (!this->read_reg16(REG_ADC_RES_L, value))
    return false;
  *value &= 0x0FFF;  // 12-bit result
  return true;
}

// ============================================================================
// Power source
// ============================================================================

bool M5PM1Component::get_power_source(PowerSource *src) {
  uint8_t val = 0;
  if (!this->read_reg(REG_PWR_SRC, &val))
    return false;
  *src = (PowerSource) (val & 0x07);
  return true;
}

// ============================================================================
// Holds
// ============================================================================

bool M5PM1Component::set_gpio_hold(uint8_t pin, bool en) {
  if (pin > 4)
    return false;
  return this->update_bits(REG_HOLD_CFG, (uint8_t) (1 << pin), en ? (uint8_t) (1 << pin) : 0);
}

// ============================================================================
// Battery LVP
// ============================================================================

bool M5PM1Component::set_battery_lvp(uint16_t mv) {
  if (mv < 2000 || mv > 4000) {
    ESP_LOGW(TAG, "Battery LVP %u mV out of range (2000-4000)", mv);
    return false;
  }
  // value = (mv - 2000) / 7.81  ->  (mv - 2000) * 100 / 781
  uint8_t lvp = (uint8_t) ((mv - 2000) * 100 / 781);
  return this->write_reg(REG_BATT_LVP, lvp);
}

// ============================================================================
// Timer
// ============================================================================

bool M5PM1Component::timer_set(uint32_t seconds, TimerAction action) {
  if (seconds > 214748364)
    return false;
  uint8_t data[4] = {
      (uint8_t) (seconds & 0xFF),
      (uint8_t) ((seconds >> 8) & 0xFF),
      (uint8_t) ((seconds >> 16) & 0xFF),
      (uint8_t) ((seconds >> 24) & 0x7F),
  };
  if (!this->write_bytes_reg(REG_TIM_CNT_0, data, 4))
    return false;
  uint8_t cfg = (uint8_t) (0x08 | (uint8_t) action);  // ARM=1, set action
  if (!this->write_reg(REG_TIM_CFG, cfg))
    return false;
  return this->write_reg(REG_TIM_KEY, TIM_RELOAD_KEY);
}

bool M5PM1Component::timer_clear() {
  if (!this->write_reg(REG_TIM_CFG, 0))
    return false;
  return this->write_reg(REG_TIM_KEY, TIM_RELOAD_KEY);
}

// ============================================================================
// GPIO
// ============================================================================

bool M5PM1Component::gpio_set_func(uint8_t pin, GpioFunc func) {
  if (pin > 4)
    return false;
  uint8_t reg = (pin < 4) ? REG_GPIO_FUNC0 : REG_GPIO_FUNC1;
  uint8_t shift = (pin < 4) ? (uint8_t) (pin * 2) : (uint8_t) ((pin - 4) * 2);
  return this->update_bits(reg, (uint8_t) (0x03 << shift), (uint8_t) (func << shift));
}

bool M5PM1Component::gpio_set_mode(uint8_t pin, bool output) {
  if (pin > 4)
    return false;
  return this->update_bits(REG_GPIO_MODE, (uint8_t) (1 << pin), output ? (uint8_t) (1 << pin) : 0);
}

bool M5PM1Component::gpio_set_output(uint8_t pin, bool level) {
  if (pin > 4)
    return false;
  return this->update_bits(REG_GPIO_OUT, (uint8_t) (1 << pin), level ? (uint8_t) (1 << pin) : 0);
}

bool M5PM1Component::gpio_get_input(uint8_t pin, bool *level) {
  if (pin > 4)
    return false;
  uint8_t val = 0;
  if (!this->read_reg(REG_GPIO_IN, &val))
    return false;
  *level = ((val >> pin) & 0x01) != 0;
  return true;
}

bool M5PM1Component::gpio_set_pull(uint8_t pin, GpioPull pull) {
  if (pin > 4)
    return false;
  uint8_t reg = (pin < 4) ? REG_GPIO_PUPD0 : REG_GPIO_PUPD1;
  uint8_t shift = (pin < 4) ? (uint8_t) (pin * 2) : (uint8_t) ((pin - 4) * 2);
  return this->update_bits(reg, (uint8_t) (0x03 << shift), (uint8_t) (pull << shift));
}

bool M5PM1Component::gpio_set_drive(uint8_t pin, GpioDrive drive) {
  if (pin > 4)
    return false;
  return this->update_bits(REG_GPIO_DRV, (uint8_t) (1 << pin), drive ? (uint8_t) (1 << pin) : 0);
}

// ============================================================================
// PWM
// ============================================================================

bool M5PM1Component::set_pwm_duty12(uint8_t channel, uint16_t duty12, bool polarity, bool enable) {
  if (channel > 1 || duty12 > 0x0FFF)
    return false;
  uint8_t reg_l = (channel == 0) ? REG_PWM0_L : REG_PWM1_L;
  uint8_t data_h = (uint8_t) ((duty12 >> 8) & 0x0F);
  if (polarity)
    data_h |= 0x20;
  if (enable)
    data_h |= 0x10;
  uint8_t buf[2] = {(uint8_t) (duty12 & 0xFF), data_h};
  return this->write_bytes_reg(reg_l, buf, 2);
}

// ============================================================================
// Button
// ============================================================================

bool M5PM1Component::btn_get_state(bool *pressed) {
  uint8_t val = 0;
  if (!this->read_reg(REG_BTN_STATUS, &val))
    return false;
  *pressed = (val & 0x01) != 0;
  return true;
}

// ============================================================================
// NeoPixel
// ============================================================================

bool M5PM1Component::set_led_count(uint8_t count) {
  if (count == 0 || count > MAX_LED_COUNT)
    return false;
  return this->write_reg(REG_NEO_CFG, (uint8_t) (count & NEO_CFG_COUNT_MASK));
}

bool M5PM1Component::set_led_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
  if (index >= MAX_LED_COUNT)
    return false;
  uint16_t rgb565 = (uint16_t) (((uint16_t) (r & 0xF8) << 8) | ((uint16_t) (g & 0xFC) << 3) | (b >> 3));
  return this->write_reg16((uint8_t) (REG_NEO_DATA_START + index * 2), rgb565);
}

bool M5PM1Component::refresh_leds() {
  uint8_t cfg = 0;
  if (!this->read_reg(REG_NEO_CFG, &cfg))
    return false;
  cfg |= NEO_CFG_REFRESH;
  return this->write_reg(REG_NEO_CFG, cfg);
}

// ============================================================================
// AW8737A audio amplifier gain pulse
// ============================================================================

bool M5PM1Component::aw8737a_pulse(uint8_t pin, Aw8737aPulse num, bool refresh_now) {
  if (pin > 4)
    return false;
  uint8_t val = (uint8_t) (((uint8_t) num << 5) | (pin & 0x1F));
  if (refresh_now)
    val |= 0x80;  // REFRESH
  if (!this->write_reg(REG_AW8737A_PULSE, val))
    return false;
  if (refresh_now)
    delay(20);
  return true;
}

// ============================================================================
// I2C runtime config
// ============================================================================

bool M5PM1Component::set_i2c_sleep_time(uint8_t seconds) {
  if (seconds > 15)
    return false;
  return this->update_bits(REG_I2C_CFG, I2C_CFG_SLEEP_MASK, seconds);
}

bool M5PM1Component::switch_i2c_speed_400k(bool fast) {
  // Note: only writes the PMIC-side config bit. The ESPHome I2C bus speed is
  // configured separately in the `i2c:` block and is not changed here.
  return this->update_bits(REG_I2C_CFG, I2C_CFG_SPEED_400K, fast ? I2C_CFG_SPEED_400K : 0);
}

// ============================================================================
// System commands
// ============================================================================

bool M5PM1Component::sys_cmd(SysCmd cmd) {
  delay(120);
  uint8_t val = (uint8_t) (SYS_CMD_KEY | (uint8_t) cmd);
  return this->write_reg(REG_SYS_CMD, val);
}

}  // namespace m5pm1
}  // namespace esphome
