#include "m5pm1.h"
#include "esphome/core/log.h"

namespace esphome {
namespace m5pm1 {

static const char *const TAG = "m5pm1";

void M5PM1Component::setup() {
  ESP_LOGCONFIG(TAG, "M5PM1 @ 0x%02X", this->address_);
  // read_reg_ handles wake-up automatically now
}

void M5PM1Component::update() {
  read_sensors_();
  update_power_level_();
}

void M5PM1Component::trigger_update() {
  read_sensors_();
}

void M5PM1Component::read_sensors_() {
  if (battery_voltage_sensor_) {
    uint16_t voltage_mv = read_battery_voltage_mv_();
    float voltage = voltage_mv / 1000.0f;
    battery_voltage_sensor_->publish_state(voltage);
  }

  if (on_battery_sensor_ || on_usb_sensor_) {
    uint8_t src = read_reg_(REG_PWR_SRC);
    bool on_bat = (src == 2);
    bool on_usb = (src == 0) || (src == 1);
    if (on_battery_sensor_) on_battery_sensor_->publish_state(on_bat);
    if (on_usb_sensor_) on_usb_sensor_->publish_state(on_usb);
  }
}

void M5PM1Component::update_power_level_() {
  uint8_t pwr_cfg = read_reg_(REG_PWR_CFG);
  uint8_t gpio_out = read_reg_(REG_GPIO_OUT);
  ESP_LOGD(TAG, "PWR_CFG=0x%02X GPIO_OUT=0x%02X", pwr_cfg, gpio_out);
  
  bool dcdc_en = pwr_cfg & PWR_CFG_DCDC_EN;   // DCDC3V3 - ESP32 power
  bool ldo_en = pwr_cfg & PWR_CFG_LDO_EN;    // LDO3V3 - IMU power  
  bool boost_en = pwr_cfg & PWR_CFG_BOOST_EN;  // BOOST5V - 5V output
  bool gpio2_out = gpio_out & (1 << 2);     // GPIO2: LOW=L3B, HIGH=L3A
  bool charging_en = pwr_cfg & PWR_CFG_CHG_EN;

  const char* level = "L0";

  // L0: Only PMIC powered, everything off (CHG may be on)
  if (!dcdc_en && !ldo_en && charging_en) {
    level = "L0";
  }
  // L1: LDO enabled (IMU), ESP32 off
  else if (ldo_en && !dcdc_en && !gpio2_out) {
    level = "L1";
  }
  // L3A: DCDC on + GPIO2 LOW (ESP32 active, peripherals on)
  else if (dcdc_en && !gpio2_out) {
    level = "L3A";
  }
  // L3B: DCDC on + GPIO2 HIGH (all peripherals on)
  else if (dcdc_en && gpio2_out) {
    level = "L3B";
  }
  // Fallback
  else {
    level = "LX";
  }

  power_level_ = level;
  ESP_LOGD(TAG, "Power level set to: %s", level);
}

void M5PM1Component::dump_config() {
  ESP_LOGCONFIG(TAG, "M5PM1:");
  LOG_I2C_DEVICE(this);
  ESP_LOGD(TAG, " Update interval: %u ms", this->update_interval_);
  uint8_t pwr_src = read_reg_(REG_PWR_SRC);
  uint8_t pwr_cfg = read_reg_(REG_PWR_CFG);
  ESP_LOGD(TAG, "PWR_SRC=0x%02X PWR_CFG=0x%02X", pwr_src, pwr_cfg);
}

uint8_t M5PM1Component::read_reg_(uint8_t reg) {
  uint8_t val = 0;
  
  // Wake-up: read REG 0x06 first to wake PM1 from sleep
  //uint8_t wake = 0;
  //this->read_register(0x06, &wake, 1);
  
  // Now do intended read 
  if (this->read_register(reg, &val, 1) != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "I2C read failed: 0x%02X", reg);
  }
  return val;
}

void M5PM1Component::write_reg_(uint8_t reg, uint8_t val) {
  // Wake-up read first
  //uint8_t wake = 0;
  //this->read_register(0x06, &wake, 1);
  
  // Now do intended write
  if (this->write_register(reg, &val, 1) != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "I2C write failed: 0x%02X", reg);
  }
}

uint16_t M5PM1Component::read_battery_voltage_mv_() {
  uint8_t buf[2] = {0};
  this->read_register(REG_VBAT_L, buf, 2);
  return (buf[1] << 8) | buf[0];
}

void M5PM1Component::set_ldo_enabled(bool on) {
  uint8_t cfg = read_reg_(REG_PWR_CFG);
  if (on) cfg |= PWR_CFG_LDO_EN;
  else cfg &= ~PWR_CFG_LDO_EN;
  write_reg_(REG_PWR_CFG, cfg);
}
void M5PM1Component::set_dcdc_enabled(bool on) {
  uint8_t cfg = read_reg_(REG_PWR_CFG);
  if (on) cfg |= PWR_CFG_DCDC_EN;
  else cfg &= ~PWR_CFG_DCDC_EN;
  write_reg_(REG_PWR_CFG, cfg);
}
void M5PM1Component::set_5v_enabled(bool on) {
  uint8_t cfg = read_reg_(REG_PWR_CFG);
  if (on) cfg |= PWR_CFG_BOOST_EN;
  else cfg &= ~PWR_CFG_BOOST_EN;
  write_reg_(REG_PWR_CFG, cfg);
}
void M5PM1Component::set_charging_enabled(bool on) {
  uint8_t cfg = read_reg_(REG_PWR_CFG);
  if (on) cfg |= PWR_CFG_CHG_EN;
  else cfg &= ~PWR_CFG_CHG_EN;
  write_reg_(REG_PWR_CFG, cfg);
}

void M5PM1Component::set_standby(bool on) {
  // L3A = active (LOW power), L3B = all peripherals (HIGH power)
  // Control via M5PM1 GPIO2 (PYG2 pin)
  // Set GPIO2 function: REG 0x16 bits [5:4] = 0 (GPIO function)
  uint8_t func = read_reg_(REG_GPIO_FUNC0);
  func &= ~(0b11 << 4);  // Clear bits 4-5 for GPIO2 function
  func |= (GPIO_FUNC_GPIO << 4);
  write_reg_(REG_GPIO_FUNC0, func);
  
  // Set GPIO2 mode: REG 0x10 bit 2 = 1 (output)
  uint8_t mode = read_reg_(REG_GPIO_MODE);
  mode |= (1 << 2);   // output
  write_reg_(REG_GPIO_MODE, mode);
  
  // Set GPIO2 drive: REG 0x13 bit 2 = 0 (push-pull)
  uint8_t drv = read_reg_(REG_GPIO_DRV);
  drv &= ~(1 << 2);   // push-pull
  write_reg_(REG_GPIO_DRV, drv);
  
  // Set output level via REG 0x11 bit 2
  // GPIO2 HIGH = L3B (all peripherals), GPIO2 LOW = L3A (ESP32 active)
  // on=true (standby) = L3A, on=false (normal) = L3B
  uint8_t out = read_reg_(REG_GPIO_OUT);
  if (on) {
    out &= ~(1 << 2);   // L3A = standby (ESP32 active, peripherals minimal)
  } else {
    out |= (1 << 2);    // L3B = normal (all peripherals on)
  }
  write_reg_(REG_GPIO_OUT, out);
  
  ESP_LOGD(TAG, "Standby: %s (L3%s)", on ? "ON" : "OFF", on ? "A" : "B");
}

void M5PM1Component::shutdown(bool hold) {
  (void)hold;
  write_reg_(REG_SYS_CMD, (0xA << 4) | 0x01);
}

}  // namespace m5pm1
}  // namespace esphome

