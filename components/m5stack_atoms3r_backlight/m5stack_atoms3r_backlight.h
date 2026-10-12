#pragma once

#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/i2c/i2c.h"

class M5StackAtomS3RBacklight : public Component, public output::FloatOutput {
	private:
		esphome::i2c::I2CDevice *i2c_device_;

	public:
		void set_i2c_device(esphome::i2c::I2CDevice *i2c_device) { i2c_device_ = i2c_device; }

	void setup() override {
		uint8_t data;
		data = 0b01000000;
		i2c_device_->write_register(0x00, &data, 1);
		delay(1);
		data = 0b00000001;
		i2c_device_->write_register(0x08, &data, 1);
		data = 0b00000000;
		i2c_device_->write_register(0x70, &data, 1);

		// Set initial brightness
		write_state(1.0);
	}

	void write_state(float state) override {
		uint8_t brightness = static_cast<uint8_t>(state * 255);
		i2c_device_->write_register(0x0e, &brightness, 1);
	}
};