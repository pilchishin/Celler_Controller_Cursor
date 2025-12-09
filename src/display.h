#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "sensors.h"
#include "config.h"

// Обёртка над LCD 1602 (I2C) для страниц статуса/ошибок.
class Display {
public:
  bool begin();
  void backlight(bool on);
  void showStatusPage(uint8_t page, const FilteredReadings &f, bool fanOn, bool ozOn);
  void showError(const char *msg);
private:
  LiquidCrystal_I2C lcd{I2C_LCD_ADDR, 16, 2};
};


