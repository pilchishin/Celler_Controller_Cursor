#pragma once

#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "climate.h"
#include "ozone.h"

// Состояния верхнего автомата управления.
enum SystemState : uint8_t {
  ST_IDLE = 0,
  ST_AUTO,
  ST_OZONE_START,
  ST_OZONE_ACTIVE,
  ST_OZONE_HOLD,
  ST_OZONE_VENT,
  ST_ERROR,
  ST_MANUAL_FAN,
  ST_MANUAL_OZONE
};

// Описание ручных запросов пользователя.
struct ManualRequest {
  bool fan{false};
  bool ozone{false};
  uint16_t minutes{0};
  uint32_t startedAt{0};
};

// Главный FSM: климат, озон, ручные режимы, ошибки.
class FSM {
public:
  void begin();
  void setConfig(Config *cfg);
  void setStats(Stats *stats);
  void tick(uint32_t nowMs, uint32_t epoch, Sensors &sensors, ClimateController &climate, OzoneController &ozone);
  void requestManualFan(uint16_t minutes, uint32_t nowMs);
  void requestManualOzone(uint16_t minutes, uint32_t nowMs);
  void startScheduledOzone(uint32_t nowMs);
  void abortOzone();
  SystemState state() const { return state_; }
  bool relayFan() const;
  bool relayOzone() const;
  bool backlightBlock() const { return backlightOn_; }
  void setBacklight(bool on, uint32_t nowMs);

private:
  void enter(SystemState st);
  void handleOzoneTransitions(uint32_t nowMs, uint32_t epoch, const FilteredReadings &f, Sensors &sensors, OzoneController &ozone);
  void handleManual(uint32_t nowMs);
  bool outdoorOk(const FilteredReadings &f) const;
  bool condensationRisk(const FilteredReadings &f) const;

  SystemState state_{ST_IDLE};
  Config *cfg_{nullptr};
  Stats *stats_{nullptr};
  ManualRequest manual_{};
  bool fanRelay_{false};
  bool ozoneRelay_{false};
  bool backlightOn_{false};
  uint32_t backlightChanged_{0};
};


