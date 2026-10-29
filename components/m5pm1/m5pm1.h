#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace m5pm1 {

// ============================================================================
// Register map (from M5Stack M5PM1 driver, src/M5PM1.h)
// PM1 is a multi-function power management IC (I2C, default addr 0x6E).
// ============================================================================

static const uint8_t M5PM1_DEFAULT_ADDR = 0x6E;

// System registers
static const uint8_t REG_DEVICE_ID = 0x00;
static const uint8_t REG_DEVICE_MODEL = 0x01;
static const uint8_t REG_HW_REV = 0x02;
static const uint8_t REG_SW_REV = 0x03;

// Power status
static const uint8_t REG_PWR_SRC = 0x04;   // [2:0] 0=5VIN 1=5VINOUT 2=BAT
static const uint8_t REG_WAKE_SRC = 0x05;  // [6:0] wake flags (write 0 to clear)

// Power configuration
static const uint8_t REG_PWR_CFG = 0x06;   // [4]LED_CTRL [3]BOOST [2]LDO [1]DCDC [0]CHG
static const uint8_t REG_HOLD_CFG = 0x07;  // [6]BOOST [5]LDO [4:0]GPIO4..0 hold
static const uint8_t REG_BATT_LVP = 0x08;  // (mV-2000)/7.81
static const uint8_t REG_I2C_CFG = 0x09;   // [4]SPD 1=400k [3:0]sleep timeout s

// Watchdog / system command
static const uint8_t REG_WDT_CNT = 0x0A;  // countdown seconds, 0=off
static const uint8_t REG_WDT_KEY = 0x0B;  // write 0xA5 to feed
static const uint8_t REG_SYS_CMD = 0x0C;  // key 0xA in high nibble | cmd

// GPIO
static const uint8_t REG_GPIO_MODE = 0x10;      // [4:0] 1=output 0=input
static const uint8_t REG_GPIO_OUT = 0x11;       // [4:0] output level
static const uint8_t REG_GPIO_IN = 0x12;        // [4:0] input level (R)
static const uint8_t REG_GPIO_DRV = 0x13;       // [5]LED_EN drv [4:0] 1=open-drain
static const uint8_t REG_GPIO_PUPD0 = 0x14;     // 2 bits/pin, GPIO0-3
static const uint8_t REG_GPIO_PUPD1 = 0x15;     // 2 bits, GPIO4
static const uint8_t REG_GPIO_FUNC0 = 0x16;     // 2 bits/pin, GPIO0-3
static const uint8_t REG_GPIO_FUNC1 = 0x17;     // 2 bits, GPIO4
static const uint8_t REG_GPIO_WAKE_EN = 0x18;   // [4:0] wake enable
static const uint8_t REG_GPIO_WAKE_CFG = 0x19;  // [4:0] 1=rising 0=falling

// Voltage / ADC
static const uint8_t REG_VREF_L = 0x20;
static const uint8_t REG_VBAT_L = 0x22;
static const uint8_t REG_VIN_L = 0x24;
static const uint8_t REG_5VINOUT_L = 0x26;
static const uint8_t REG_ADC_RES_L = 0x28;
static const uint8_t REG_ADC_CTRL = 0x2A;  // [3:1] channel [0] start

// PWM
static const uint8_t REG_PWM0_L = 0x30;
static const uint8_t REG_PWM0_HC = 0x31;  // [5]POL [4]EN [3:0] duty high
static const uint8_t REG_PWM1_L = 0x32;
static const uint8_t REG_PWM1_HC = 0x33;
static const uint8_t REG_PWM_FREQ_L = 0x34;  // 16-bit Hz

// Timer
static const uint8_t REG_TIM_CNT_0 = 0x38;  // 4 bytes, 31-bit seconds
static const uint8_t REG_TIM_CFG = 0x3C;    // [3]ARM [2:0]action
static const uint8_t REG_TIM_KEY = 0x3D;    // write 0xA5 to reload

// IRQ
static const uint8_t REG_IRQ_STATUS1 = 0x40;  // GPIO
static const uint8_t REG_IRQ_STATUS2 = 0x41;  // system
static const uint8_t REG_IRQ_STATUS3 = 0x42;  // button
static const uint8_t REG_IRQ_MASK1 = 0x43;
static const uint8_t REG_IRQ_MASK2 = 0x44;
static const uint8_t REG_IRQ_MASK3 = 0x45;

// Button
static const uint8_t REG_BTN_STATUS = 0x48;  // [7]flag(auto-clear on read) [0]state
static const uint8_t REG_BTN_CFG_1 = 0x49;   // [7]DL_LOCK ... [0]single-reset-dis
static const uint8_t REG_BTN_CFG_2 = 0x4A;   // [0]double-off-dis

// NeoPixel
static const uint8_t REG_NEO_CFG = 0x50;         // [6]refresh [5:0]count
static const uint8_t REG_AW8737A_PULSE = 0x53;   // [7]refresh [6:5]num [4:0]gpio
static const uint8_t REG_NEO_DATA_START = 0x60;  // 2 bytes/LED RGB565
static const uint8_t REG_RTC_RAM_START = 0xA0;   // 32 bytes retained

// Bit helpers
static const uint8_t PWR_CFG_CHG_EN = 1 << 0;
static const uint8_t PWR_CFG_DCDC_EN = 1 << 1;
static const uint8_t PWR_CFG_LDO_EN = 1 << 2;
static const uint8_t PWR_CFG_BOOST_EN = 1 << 3;
static const uint8_t PWR_CFG_LED_CTRL = 1 << 4;

static const uint8_t HOLD_LDO = 1 << 5;
static const uint8_t HOLD_BOOST = 1 << 6;

static const uint8_t I2C_CFG_SLEEP_MASK = 0x0F;
static const uint8_t I2C_CFG_SPEED_400K = 1 << 4;

static const uint8_t NEO_CFG_REFRESH = 1 << 6;
static const uint8_t NEO_CFG_COUNT_MASK = 0x3F;

static const uint8_t SYS_CMD_KEY = 0xA0;
static const uint8_t WDT_FEED_KEY = 0xA5;
static const uint8_t TIM_RELOAD_KEY = 0xA5;

static const uint8_t MAX_LED_COUNT = 32;

// ============================================================================
// Enumerations mirroring the vendor API
// ============================================================================

enum PowerSource : uint8_t {
  PWR_SRC_NONE = 0,
  PWR_SRC_5VIN = 1 << 0,     // 0x01
  PWR_SRC_5VINOUT = 1 << 1,  // 0x02
  PWR_SRC_BAT = 1 << 2,      // 0x04
};

enum AdcChannel : uint8_t {
  ADC_CH_1 = 1,     // GPIO1
  ADC_CH_2 = 2,     // GPIO2
  ADC_CH_TEMP = 6,  // internal temperature (0.1 °C)
};

enum GpioFunc : uint8_t {
  GPIO_FUNC_GPIO = 0b00,
  GPIO_FUNC_IRQ = 0b01,
  GPIO_FUNC_WAKE = 0b10,
  GPIO_FUNC_OTHER = 0b11,  // LED_EN / ADC / PWM
};

enum GpioPull : uint8_t {
  GPIO_PULL_NONE = 0b00,
  GPIO_PULL_UP = 0b01,
  GPIO_PULL_DOWN = 0b10,
};

enum GpioDrive : uint8_t {
  GPIO_DRIVE_PUSHPULL = 0,
  GPIO_DRIVE_OPENDRAIN = 1,
};

enum SysCmd : uint8_t {
  SYS_CMD_OFF = 0x01,
  SYS_CMD_RESET = 0x02,
  SYS_CMD_DL = 0x03,
};

enum TimerAction : uint8_t {
  TIM_ACTION_STOP = 0b000,
  TIM_ACTION_FLAG = 0b001,
  TIM_ACTION_REBOOT = 0b010,
  TIM_ACTION_POWERON = 0b011,
  TIM_ACTION_POWEROFF = 0b100,
};

enum Aw8737aPulse : uint8_t {
  AW8737A_PULSE_0 = 0,
  AW8737A_PULSE_1 = 1,
  AW8737A_PULSE_2 = 2,
  AW8737A_PULSE_3 = 3,
};

// ============================================================================
// Hub component
// ============================================================================

class M5PM1Component : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::BUS - 1.0f; }

  // ---- boot-time configuration (from YAML) ----
  void set_charge_boot(int v) { this->charge_boot_ = v; }
  void set_dcdc_boot(int v) { this->dcdc_boot_ = v; }
  void set_ldo_boot(int v) { this->ldo_boot_ = v; }
  void set_boost_boot(int v) { this->boost_boot_ = v; }
  void set_l3b_boot(int v) { this->l3b_boot_ = v; }
  void set_battery_lvp_boot(int mv) { this->battery_lvp_boot_ = mv; }
  void set_i2c_sleep_boot(int s) { this->i2c_sleep_boot_ = s; }

  // ---- low-level register access (return true on success) ----
  bool read_reg(uint8_t reg, uint8_t *value);
  bool write_reg(uint8_t reg, uint8_t value);
  bool read_reg16(uint8_t reg, uint16_t *value);   // little-endian
  bool write_reg16(uint8_t reg, uint16_t value);    // little-endian
  bool read_bytes_reg(uint8_t reg, uint8_t *data, uint8_t len);
  bool write_bytes_reg(uint8_t reg, const uint8_t *data, uint8_t len);
  bool update_bits(uint8_t reg, uint8_t mask, uint8_t value);

  // ---- device info ----
  bool get_device_id(uint8_t *id) { return this->read_reg(REG_DEVICE_ID, id); }

  // ---- voltage / ADC / temperature ----
  bool read_vref(uint16_t *mv) { return this->read_reg16(REG_VREF_L, mv); }
  bool read_vbat(uint16_t *mv) { return this->read_reg16(REG_VBAT_L, mv); }
  bool read_vin(uint16_t *mv) { return this->read_reg16(REG_VIN_L, mv); }
  bool read_5vinout(uint16_t *mv) { return this->read_reg16(REG_5VINOUT_L, mv); }
  bool read_adc(AdcChannel channel, uint16_t *value);
  bool read_temperature_deci(uint16_t *deci_c) { return this->read_adc(ADC_CH_TEMP, deci_c); }

  // ---- power source / wake source ----
  bool get_power_source(PowerSource *src);
  bool get_wake_source(uint8_t *flags) { return this->read_reg(REG_WAKE_SRC, flags); }

  // ---- power rails / holds ----
  bool set_power_config(uint8_t mask, uint8_t value) { return this->update_bits(REG_PWR_CFG, mask, value); }
  bool get_power_config(uint8_t *cfg) { return this->read_reg(REG_PWR_CFG, cfg); }
  bool set_charge_enable(bool en) { return this->set_power_config(PWR_CFG_CHG_EN, en ? PWR_CFG_CHG_EN : 0); }
  bool set_dcdc_enable(bool en) { return this->set_power_config(PWR_CFG_DCDC_EN, en ? PWR_CFG_DCDC_EN : 0); }
  bool set_ldo_enable(bool en) { return this->set_power_config(PWR_CFG_LDO_EN, en ? PWR_CFG_LDO_EN : 0); }
  bool set_boost_enable(bool en) { return this->set_power_config(PWR_CFG_BOOST_EN, en ? PWR_CFG_BOOST_EN : 0); }
  bool set_led_en_level(bool high) { return this->set_power_config(PWR_CFG_LED_CTRL, high ? PWR_CFG_LED_CTRL : 0); }
  bool get_hold_config(uint8_t *cfg) { return this->read_reg(REG_HOLD_CFG, cfg); }
  bool set_ldo_hold(bool en) { return this->update_bits(REG_HOLD_CFG, HOLD_LDO, en ? HOLD_LDO : 0); }
  bool set_boost_hold(bool en) { return this->update_bits(REG_HOLD_CFG, HOLD_BOOST, en ? HOLD_BOOST : 0); }
  bool set_gpio_hold(uint8_t pin, bool en);

  // ---- battery ----
  bool set_battery_lvp(uint16_t mv);

  // ---- watchdog ----
  bool wdt_set(uint8_t timeout_sec) { return this->write_reg(REG_WDT_CNT, timeout_sec); }
  bool wdt_feed() { return this->write_reg(REG_WDT_KEY, WDT_FEED_KEY); }
  bool wdt_get_count(uint8_t *count) { return this->read_reg(REG_WDT_CNT, count); }

  // ---- timer ----
  bool timer_set(uint32_t seconds, TimerAction action);
  bool timer_clear();

  // ---- GPIO ----
  bool gpio_set_func(uint8_t pin, GpioFunc func);
  bool gpio_set_mode(uint8_t pin, bool output);
  bool gpio_set_output(uint8_t pin, bool level);
  bool gpio_get_input(uint8_t pin, bool *level);
  bool gpio_set_pull(uint8_t pin, GpioPull pull);
  bool gpio_set_drive(uint8_t pin, GpioDrive drive);

  // ---- PWM ----
  bool set_pwm_frequency(uint16_t hz) { return this->write_reg16(REG_PWM_FREQ_L, hz); }
  bool get_pwm_frequency(uint16_t *hz) { return this->read_reg16(REG_PWM_FREQ_L, hz); }
  bool set_pwm_duty12(uint8_t channel, uint16_t duty12, bool polarity, bool enable);

  // ---- button ----
  bool btn_get_state(bool *pressed);

  // ---- NeoPixel ----
  bool set_led_count(uint8_t count);
  bool set_led_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
  bool refresh_leds();
  bool disable_leds() { return this->write_reg(REG_NEO_CFG, 0); }

  // ---- AW8737A audio amp pulse ----
  bool aw8737a_pulse(uint8_t pin, Aw8737aPulse num, bool refresh_now);

  // ---- I2C runtime config ----
  bool set_i2c_sleep_time(uint8_t seconds);
  bool switch_i2c_speed_400k(bool fast);

  // ---- system commands ----
  bool sys_cmd(SysCmd cmd);
  bool shutdown_pmic() { return this->sys_cmd(SYS_CMD_OFF); }
  bool reboot_pmic() { return this->sys_cmd(SYS_CMD_RESET); }
  bool enter_download_mode() { return this->sys_cmd(SYS_CMD_DL); }

 protected:
  int charge_boot_{-1};
  int dcdc_boot_{-1};
  int ldo_boot_{-1};
  int boost_boot_{-1};
  int l3b_boot_{-1};
  int battery_lvp_boot_{-1};
  int i2c_sleep_boot_{-1};
};

}  // namespace m5pm1
}  // namespace esphome
