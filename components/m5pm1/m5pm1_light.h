#pragma once

#include "esphome/core/defines.h"
#ifdef USE_LIGHT

#include "esphome/core/component.h"
#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/light_output.h"
#include "m5pm1.h"

namespace esphome {
namespace m5pm1 {

// Addressable light backed by the PM1's built-in NeoPixel controller (RGB565,
// up to 32 LEDs). Pixel data is buffered locally and pushed over I2C on write.
class M5PM1Light : public light::AddressableLight {
 public:
  void setup() override;
  void write_state(light::LightState *state) override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  int32_t size() const override { return this->num_leds_; }
  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::RGB});
    return traits;
  }
  void clear_effect_data() override {
    for (int i = 0; i < this->size(); i++)
      this->effect_data_[i] = 0;
  }

  void set_parent(M5PM1Component *parent) { this->parent_ = parent; }
  void set_num_leds(uint16_t num_leds) { this->num_leds_ = num_leds; }

 protected:
  light::ESPColorView get_view_internal(int32_t index) const override;

  M5PM1Component *parent_;
  uint16_t num_leds_{1};
  uint8_t *buf_{nullptr};
  uint8_t *effect_data_{nullptr};
};

}  // namespace m5pm1
}  // namespace esphome


#endif  // USE_LIGHT
