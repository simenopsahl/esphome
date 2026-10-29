#pragma once

#include <string>
#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include <driver/i2c_master.h>

namespace esphome
{
  namespace m5pm1
  {

    class M5PM1Component : public PollingComponent, public i2c::I2CDevice
    {
    public:
      void setup() override;
      void update() override;
      void dump_config() override;

      void trigger_update();

      void set_battery_voltage_sensor(sensor::Sensor *s) { battery_voltage_sensor_ = s; }
      void set_on_battery_sensor(binary_sensor::BinarySensor *s) { on_battery_sensor_ = s; }
      void set_on_usb_sensor(binary_sensor::BinarySensor *s) { on_usb_sensor_ = s; }
      void set_update_interval(uint32_t interval) { this->update_interval_ = interval; }

      const char *get_power_level() { return power_level_.c_str(); }

      void set_ldo_enabled(bool on);
      void set_dcdc_enabled(bool on);
      void set_5v_enabled(bool on); // L3A: 5V output enable (standby ON)
      void set_charging_enabled(bool on);
      void set_standby(bool on); // L3A/L3B control
      void shutdown(bool hold = true);

    protected:
      void read_sensors_();
      void update_power_level_();
      uint8_t read_reg_(uint8_t reg);
      void write_reg_(uint8_t reg, uint8_t val);
      uint16_t read_battery_voltage_mv_();

      sensor::Sensor *battery_voltage_sensor_{nullptr};
      binary_sensor::BinarySensor *on_battery_sensor_{nullptr};
      binary_sensor::BinarySensor *on_usb_sensor_{nullptr};
      std::string power_level_{"L0"};

      static const uint8_t REG_PWR_SRC = 0x04;
      static const uint8_t REG_PWR_CFG = 0x06;
      static const uint8_t REG_GPIO_FUNC0 = 0x16;
      static const uint8_t REG_GPIO_FUNC1 = 0x17;
      static const uint8_t REG_GPIO_MODE = 0x10;
      static const uint8_t REG_GPIO_DRV = 0x13;
      static const uint8_t REG_GPIO_OUT = 0x11;
      static const uint8_t REG_GPIO_IN = 0x12;
      static const uint8_t REG_SYS_CMD = 0x0C;
      static const uint8_t REG_VBAT_L = 0x22;
      static const uint8_t REG_VBAT_H = 0x23;

      static const uint8_t PWR_CFG_CHG_EN = (1 << 0);
      static const uint8_t PWR_CFG_DCDC_EN = (1 << 1);
      static const uint8_t PWR_CFG_LDO_EN = (1 << 2);
      static const uint8_t PWR_CFG_BOOST_EN = (1 << 3);
      static const uint8_t PWR_CFG_LED_CTRL = (1 << 4);

      static const uint8_t GPIO_FUNC_GPIO = 0b00;
      static const uint8_t GPIO_FUNC_IRQ = 0b01;
      static const uint8_t GPIO_FUNC_WAKE = 0b10;
      static const uint8_t GPIO_FUNC_OTHER = 0b11;

      static const uint8_t GPIO_MODE_INPUT = 0;
      static const uint8_t GPIO_MODE_OUTPUT = 1;

      static const uint8_t GPIO_DRIVE_PUSHPULL = 0;
      static const uint8_t GPIO_DRIVE_OPENDRAIN = 1;

      static const uint8_t GPIO_STATE_LOW = 0;
      static const uint8_t GPIO_STATE_HIGH = 1;
    };

  } // namespace m5pm1
} // namespace esphome