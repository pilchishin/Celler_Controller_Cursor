#pragma once

#include <Arduino.h>

// Общие настройки пинов, дефолтных параметров и структур конфигурации.
// Pin map — сопоставление сигналов с пинами Nano.

constexpr uint8_t PIN_SSR_FAN = 3;        // Digital, SSR for intake fan
constexpr uint8_t PIN_SSR_OZONE = 4;      // Digital, SSR for ozonator
constexpr uint8_t PIN_BTN_MENU = 5;
constexpr uint8_t PIN_BTN_UP = 6;
constexpr uint8_t PIN_BTN_DOWN = 7;
constexpr uint8_t PIN_ONEWIRE = 8;      //  DS18B20 bus

// I2C addresses
constexpr uint8_t I2C_LCD_ADDR = 0x27;
constexpr uint8_t I2C_BME280_ADDR = 0x76;

// Timing — интервалы опроса и вспомогательные таймауты.
constexpr uint32_t SENSOR_PERIOD_MS = 10'000;   // 10 s
constexpr uint32_t UI_TICK_MS = 250;
constexpr uint32_t BACKLIGHT_TIMEOUT_MS = 60'000;
constexpr uint32_t STATS_SNAPSHOT_MS = 30UL * 60UL * 1000UL; // 30 min
constexpr uint32_t LOOP_BUDGET_MS = 500;

// Fan logic hysteresis — целевые значения и допуски.
constexpr float TARGET_TEMP_DEFAULT = 4.0f;
constexpr float TARGET_RH_DEFAULT = 85.0f;
constexpr float TEMP_HYST = 0.5f;
constexpr float RH_HYST = 3.0f;
constexpr float DEW_SAFETY_MARGIN = 2.0f;
constexpr float CONDENSATION_STOP_MARGIN = 4.0f;
constexpr float MIN_CELLAR_TEMP = 2.0f;

// Ozone — длительности фаз цикла.
constexpr uint16_t OZONE_ACTIVE_MIN = 15;
constexpr uint16_t OZONE_HOLD_MIN = 120;
constexpr uint16_t OZONE_VENT_MIN = 15;
constexpr uint8_t OZONE_RETRY_MIN = 30;

// Menu — параметры кнопок.
constexpr uint8_t MENU_DEBOUNCE_MS = 80;
constexpr uint8_t MENU_LONGPRESS_MS = 800;

// EEPROM layout — размеры wear-level блока и смещение статистики.
constexpr size_t EEPROM_CFG_SLOT_SIZE = 64;
constexpr uint8_t EEPROM_CFG_SLOTS = 4;
constexpr size_t EEPROM_STATS_OFFSET = EEPROM_CFG_SLOT_SIZE * EEPROM_CFG_SLOTS;

// Event codes
enum EventCode : uint8_t {
  EVT_NONE = 0,
  EVT_BOOT = 1,
  EVT_ERR_SENSOR = 2,
  EVT_ERR_CONDENSATION = 3,
  EVT_ERR_LOW_TEMP = 4,
  EVT_OZONE_START = 10,
  EVT_OZONE_ABORT = 11,
  EVT_OZONE_DONE = 12,
  EVT_MANUAL_FAN = 20,
  EVT_MANUAL_OZONE = 21,
  EVT_CFG_CHANGE = 30
};

struct OzoneSchedule {
  bool enabled{true};
  uint8_t weekday{1}; // Monday=1..7
  uint8_t hour{2};
  uint8_t minute{0};
};

struct SensorOffsets {
  float bmeTemp{0};
  float bmeRh{0};
  float ds18b20{0};
  float htuTemp{0};
  float htuRh{0};
};

struct Config {
  float targetTemp{TARGET_TEMP_DEFAULT};
  float targetRh{TARGET_RH_DEFAULT};
  SensorOffsets offsets{};
  OzoneSchedule ozone{};
  bool debug{false};
  uint16_t crc{0};
};

struct Stats {
  uint32_t fanMinutes{0};
  uint32_t ozoneMinutes{0};
  uint32_t lastSnapshotTs{0};
  uint8_t logHead{0};
};

struct EventEntry {
  uint8_t code{0};
  uint32_t ts{0};
};

constexpr uint8_t EVENT_LOG_SIZE = 20;


