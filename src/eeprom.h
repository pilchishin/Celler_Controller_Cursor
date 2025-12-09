#pragma once

#include <Arduino.h>
#include "config.h"

// Хранилище конфигурации и статистики с wear-level и CRC.
class EepromStore {
public:
  bool begin();
  Config loadConfig();
  void saveConfig(const Config &cfg);
  Stats loadStats();
  void saveStats(const Stats &st);
  void logEvent(EventCode code, uint32_t ts);
  EventEntry readEvent(uint8_t idx);

private:
  uint8_t activeSlot();
  void writeSlot(uint8_t slot, const Config &cfg);
};


