#pragma once

#include <Arduino.h>
#include "display.h"
#include "config.h"
#include "fsm.h"
#include "eeprom.h"

// Страницы меню (упрощённые заглушки).
enum MenuPage : uint8_t {
  PAGE_STATUS = 0,
  PAGE_MANUAL_FAN,
  PAGE_MANUAL_OZONE,
  PAGE_SETTINGS,
  PAGE_MAX
};

// Менеджер меню, обрабатывает кнопки и перерисовку дисплея.
class Menu {
public:
  void begin(Display *disp, FSM *fsm, EepromStore *store, Config *cfg);
  void tick(uint32_t nowMs, const FilteredReadings &f, bool fanOn, bool ozOn);
  void handleButton(uint8_t btn, bool longPress, uint32_t nowMs);
  bool backlightOn() const { return backlight_; }

private:
  void render(const FilteredReadings &f, bool fanOn, bool ozOn);
  void nextPage();
  void prevPage();
  void applyConfigChange();

  Display *disp_{nullptr};
  FSM *fsm_{nullptr};
  EepromStore *store_{nullptr};
  Config *cfg_{nullptr};
  MenuPage page_{PAGE_STATUS};
  bool backlight_{true};
  uint8_t statusPage_{0};
};


