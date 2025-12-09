#pragma once

#include <Arduino.h>

// Простая структура временных меток для кооперативного планировщика.
struct Scheduler {
  uint32_t lastSensors{0};
  uint32_t lastUi{0};
  uint32_t lastStats{0};
};

// Проверка наступления периода с обновлением last.
inline bool due(uint32_t now, uint32_t &last, uint32_t period) {
  if (now - last >= period) {
    last = now;
    return true;
  }
  return false;
}


