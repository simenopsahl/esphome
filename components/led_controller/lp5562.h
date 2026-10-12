/**
 * @file lp5562.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2024-07-30
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
// #include <mooncake.h>
#include <M5Unified.h>
#include <driver/gpio.h>
// https://www.ti.com/lit/ds/symlink/lp5562.pdf?ts=1722311496082
// https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/leds/leds-lp5562.c

class LP5562_Class : public m5::I2C_Device
{
private:
    bool _write_register(uint8_t reg, uint8_t value);
    uint8_t _read_register(uint8_t reg);
    void _delay(uint32_t ms);

public:
    LP5562_Class(std::uint8_t i2c_addr = 0x30, std::uint32_t freq = 400000, m5::I2C_Class* i2c = &m5::In_I2C)
        : I2C_Device(i2c_addr, freq, i2c)
    {
    }

    bool begin();
    void setLedCurrent(uint8_t channel, uint8_t current);
    void setBrightness(uint8_t channel, uint8_t brightness);
    void reset();
    void enable();
    void disable();
    void runProgram();
    void stopProgram();
};