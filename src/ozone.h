#pragma once

#include <Arduino.h>
#include "config.h"
#include "sensors.h"

// Состояния фаз озонирования/проветривания.
enum OzoneState : uint8_t {
  OZ_IDLE = 0,
  OZ_ACTIVE,
  OZ_HOLD,
  OZ_VENT,
  OZ_ABORT
};

// Текущее состояние цикла.
struct OzoneCycle {
  OzoneState state{OZ_IDLE};
  uint32_t stateStart{0};
  bool running{false};
  uint8_t retryCount{0};
};

// Логика запуска/переходов фаз озонирования.
class OzoneController {
public:
  void reset();
  void cancel();
  void start(uint32_t nowMs);
  void tick(uint32_t nowMs, const FilteredReadings &f, bool backlightOn, bool outdoorOk);
  OzoneCycle status() const { return cycle_; }
  bool relayOzone() const;
  bool relayFan() const;
  bool isActivePhase() const { return cycle_.state == OZ_ACTIVE; }
  bool isHoldPhase() const { return cycle_.state == OZ_HOLD; }
  bool isVentPhase() const { return cycle_.state == OZ_VENT || cycle_.state == OZ_ABORT; }

private:
  void setState(OzoneState s, uint32_t now);
  OzoneCycle cycle_{};
};


