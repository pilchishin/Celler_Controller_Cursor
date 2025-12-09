#include "eeprom.h"
#include <EEPROM.h>
#include "utils.h"

bool EepromStore::begin() {
  return true;
}

uint8_t EepromStore::activeSlot() {
  // Выбираем последний валидный слот (CRC+timestamp) среди 4 блоков.
  uint8_t best = 0;
  uint32_t bestTs = 0;
  for (uint8_t i = 0; i < EEPROM_CFG_SLOTS; ++i) {
    uint16_t crcStored;
    EEPROM.get(i * EEPROM_CFG_SLOT_SIZE + offsetof(Config, crc), crcStored);
    Config tmp;
    EEPROM.get(i * EEPROM_CFG_SLOT_SIZE, tmp);
    uint16_t crcCalc = crc16_bytes((uint8_t *)&tmp, sizeof(Config) - sizeof(uint16_t));
    if (crcCalc == crcStored) {
      uint32_t ts;
      EEPROM.get(i * EEPROM_CFG_SLOT_SIZE + sizeof(Config), ts);
      if (ts >= bestTs) {
        bestTs = ts;
        best = i;
      }
    }
  }
  return best;
}

Config EepromStore::loadConfig() {
  // Читаем активный слот; при неверном CRC — берём значения по умолчанию.
  Config cfg;
  uint8_t slot = activeSlot();
  EEPROM.get(slot * EEPROM_CFG_SLOT_SIZE, cfg);
  uint16_t crcCalc = crc16_bytes((uint8_t *)&cfg, sizeof(Config) - sizeof(uint16_t));
  if (crcCalc != cfg.crc) {
    cfg = Config(); // defaults
  }
  return cfg;
}

void EepromStore::writeSlot(uint8_t slot, const Config &cfg) {
  // Записываем конфиг и рядом сохраняем метку времени.
  EEPROM.put(slot * EEPROM_CFG_SLOT_SIZE, cfg);
  uint32_t ts = nowEpoch();
  EEPROM.put(slot * EEPROM_CFG_SLOT_SIZE + sizeof(Config), ts);
}

void EepromStore::saveConfig(const Config &cfgIn) {
  // Кольцевой wear-level: следующий слот после активного.
  Config cfg = cfgIn;
  cfg.crc = crc16_bytes((uint8_t *)&cfg, sizeof(Config) - sizeof(uint16_t));
  uint8_t slot = activeSlot();
  slot = (slot + 1) % EEPROM_CFG_SLOTS;
  writeSlot(slot, cfg);
}

Stats EepromStore::loadStats() {
  // Статистика хранится по фиксированному смещению.
  Stats st;
  EEPROM.get(EEPROM_STATS_OFFSET, st);
  return st;
}

void EepromStore::saveStats(const Stats &st) {
  EEPROM.put(EEPROM_STATS_OFFSET, st);
}

void EepromStore::logEvent(EventCode code, uint32_t ts) {
  // Кольцевой журнал последних EVENT_LOG_SIZE событий.
  Stats st = loadStats();
  EventEntry entry{code, ts};
  uint8_t head = st.logHead % EVENT_LOG_SIZE;
  size_t base = EEPROM_STATS_OFFSET + sizeof(Stats) + head * sizeof(EventEntry);
  EEPROM.put(base, entry);
  st.logHead = (head + 1) % EVENT_LOG_SIZE;
  saveStats(st);
}

EventEntry EepromStore::readEvent(uint8_t idx) {
  EventEntry e{};
  size_t base = EEPROM_STATS_OFFSET + sizeof(Stats) + idx * sizeof(EventEntry);
  EEPROM.get(base, e);
  return e;
}


