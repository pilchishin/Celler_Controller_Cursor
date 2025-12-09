#include "ozone.h"

void OzoneController::reset() {
  cycle_ = {};
}

void OzoneController::cancel() {
  cycle_.running = false;
  cycle_.state = OZ_IDLE;
}

void OzoneController::start(uint32_t nowMs) {
  cycle_.running = true;
  setState(OZ_ACTIVE, nowMs);
}

void OzoneController::tick(uint32_t nowMs, const FilteredReadings &f, bool backlightOn, bool outdoorOk) {
  // Управление переходами фаз по таймерам и условиям безопасности.
  if (!cycle_.running) return;
  switch (cycle_.state) {
    case OZ_ACTIVE:
      // Активная озонация -> выдержка.
      if (nowMs - cycle_.stateStart >= OZONE_ACTIVE_MIN * 60UL * 1000UL) {
        setState(OZ_HOLD, nowMs);
      }
      break;
    case OZ_HOLD:
      // Подсветка = люди → аварийное проветривание.
      if (backlightOn) {
        setState(OZ_ABORT, nowMs);
        break;
      }
      // Нормальный переход к проветриванию.
      if (nowMs - cycle_.stateStart >= OZONE_HOLD_MIN * 60UL * 1000UL) {
        setState(OZ_VENT, nowMs);
      }
      break;
    case OZ_VENT:
    case OZ_ABORT:
      // При проветривании требуем t_out>=0 и контроль минимальной t_in.
      if (!outdoorOk) {
        cancel();
        break;
      }
      if (!isnan(f.tin) && f.tin <= MIN_CELLAR_TEMP) {
        cancel();
        break;
      }
      if (nowMs - cycle_.stateStart >= OZONE_VENT_MIN * 60UL * 1000UL) {
        cancel();
      }
      break;
    default:
      break;
  }
}

bool OzoneController::relayOzone() const {
  return cycle_.running && cycle_.state == OZ_ACTIVE;
}

bool OzoneController::relayFan() const {
  return cycle_.running && (cycle_.state == OZ_VENT || cycle_.state == OZ_ABORT);
}

void OzoneController::setState(OzoneState s, uint32_t now) {
  cycle_.state = s;
  cycle_.stateStart = now;
}


