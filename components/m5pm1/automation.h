#pragma once

#include "esphome/core/automation.h"
#include "m5pm1.h"

namespace esphome {
namespace m5pm1 {

template<typename... Ts> class ShutdownAction : public Action<Ts...> {
 public:
  explicit ShutdownAction(M5PM1Component *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->shutdown_pmic(); }

 protected:
  M5PM1Component *parent_;
};

template<typename... Ts> class RebootAction : public Action<Ts...> {
 public:
  explicit RebootAction(M5PM1Component *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->reboot_pmic(); }

 protected:
  M5PM1Component *parent_;
};

template<typename... Ts> class DownloadModeAction : public Action<Ts...> {
 public:
  explicit DownloadModeAction(M5PM1Component *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->enter_download_mode(); }

 protected:
  M5PM1Component *parent_;
};

template<typename... Ts> class FeedWatchdogAction : public Action<Ts...> {
 public:
  explicit FeedWatchdogAction(M5PM1Component *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->wdt_feed(); }

 protected:
  M5PM1Component *parent_;
};

template<typename... Ts> class SetTimerAction : public Action<Ts...> {
 public:
  explicit SetTimerAction(M5PM1Component *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint32_t, seconds)
  void set_action(TimerAction action) { this->action_ = action; }
  void play(Ts... x) override { this->parent_->timer_set(this->seconds_.value(x...), this->action_); }

 protected:
  M5PM1Component *parent_;
  TimerAction action_{TIM_ACTION_POWERON};
};

template<typename... Ts> class SetBatteryLvpAction : public Action<Ts...> {
 public:
  explicit SetBatteryLvpAction(M5PM1Component *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint16_t, millivolts)
  void play(Ts... x) override { this->parent_->set_battery_lvp(this->millivolts_.value(x...)); }

 protected:
  M5PM1Component *parent_;
};

template<typename... Ts> class Aw8737aPulseAction : public Action<Ts...> {
 public:
  explicit Aw8737aPulseAction(M5PM1Component *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint8_t, pin)
  TEMPLATABLE_VALUE(uint8_t, pulses)
  void play(Ts... x) override {
    this->parent_->aw8737a_pulse(this->pin_.value(x...), (Aw8737aPulse) this->pulses_.value(x...), true);
  }

 protected:
  M5PM1Component *parent_;
};

}  // namespace m5pm1
}  // namespace esphome
