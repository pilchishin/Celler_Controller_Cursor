#include "fsm.h"
#include "config.h"

void FSM::begin() {
  state_ = ST_AUTO;
}

void FSM::setConfig(Config *cfg) { cfg_ = cfg; }
void FSM::setStats(Stats *stats) { stats_ = stats; }

bool FSM::outdoorOk(const FilteredReadings &f) const {
  return !isnan(f.tout) && f.tout >= 0.0f;
}

bool FSM::condensationRisk(const FilteredReadings &f) const {
  return (!isnan(f.dewIn) && !isnan(f.tin) && (f.dewIn >= f.tin - CONDENSATION_STOP_MARGIN))
      || (!isnan(f.tin) && f.tin <= MIN_CELLAR_TEMP);
}

void FSM::tick(uint32_t nowMs, uint32_t epoch, Sensors &sensors, ClimateController &climate, OzoneController &ozone) {
  // Центральный цикл автомата: решаем приоритеты климат/озон/ручные режимы.
  FilteredReadings f = sensors.filtered();
  auto dec = climate.evaluate(f, sensors.bmeStatus(), sensors.htuStatus(), sensors.dsStatus(),
                              cfg_->targetTemp, cfg_->targetRh, nowMs);

  // Backlight auto off
  if (backlightOn_ && (nowMs - backlightChanged_ > BACKLIGHT_TIMEOUT_MS)) {
    backlightOn_ = false;
  }

  // Safety errors
  if (condensationRisk(f) || (!isnan(f.tin) && f.tin <= MIN_CELLAR_TEMP) ||
      !sensors.bmeStatus().ok || !sensors.htuStatus().ok || !sensors.dsStatus().ok || !sensors.rtcStatus().ok) {
    // Любая критическая проблема — в ERROR с обесточенными реле.
    enter(ST_ERROR);
    fanRelay_ = false;
    ozoneRelay_ = false;
    return;
  }

  handleManual(nowMs);

  if (state_ == ST_MANUAL_FAN || state_ == ST_MANUAL_OZONE) {
    // Ручной режим имеет приоритет, пока не истечёт таймер.
    return;
  }

  // Ozone state management
  handleOzoneTransitions(nowMs, epoch, f, sensors, ozone);
  if (ozone.status().running) {
    fanRelay_ = ozone.relayFan();
    ozoneRelay_ = ozone.relayOzone();
    return;
  }

  // Automatic climate
  if (state_ != ST_ERROR) {
    state_ = ST_AUTO;
    fanRelay_ = dec.fanShouldRun;
    ozoneRelay_ = false;
  }
}

void FSM::handleOzoneTransitions(uint32_t nowMs, uint32_t /*epoch*/, const FilteredReadings &f,
                                 Sensors &sensors, OzoneController &ozone) {
  // Обслуживаем автомат озона и отражаем его состояние наружу.
  ozone.tick(nowMs, f, backlightOn_, outdoorOk(f));
  auto st = ozone.status();
  switch (st.state) {
    case OZ_ACTIVE: enter(ST_OZONE_ACTIVE); break;
    case OZ_HOLD: enter(ST_OZONE_HOLD); break;
    case OZ_VENT:
    case OZ_ABORT: enter(ST_OZONE_VENT); break;
    default: break;
  }
}

void FSM::requestManualFan(uint16_t minutes, uint32_t nowMs) {
  manual_.fan = true;
  manual_.ozone = false;
  manual_.minutes = minutes;
  manual_.startedAt = nowMs;
  enter(ST_MANUAL_FAN);
  fanRelay_ = true;
}

void FSM::requestManualOzone(uint16_t minutes, uint32_t nowMs) {
  manual_.ozone = true;
  manual_.fan = false;
  manual_.minutes = minutes;
  manual_.startedAt = nowMs;
  enter(ST_MANUAL_OZONE);
  ozoneRelay_ = true;
}

void FSM::startScheduledOzone(uint32_t nowMs) {
  enter(ST_OZONE_START);
  manual_ = {};
  // Actual start handled by caller to OzoneController
}

void FSM::abortOzone() {
  enter(ST_AUTO);
}

void FSM::handleManual(uint32_t nowMs) {
  if (!manual_.fan && !manual_.ozone) return;
  if (nowMs - manual_.startedAt >= manual_.minutes * 60UL * 1000UL) {
    manual_ = {};
    enter(ST_AUTO);
    fanRelay_ = false;
    ozoneRelay_ = false;
  }
}

bool FSM::relayFan() const {
  if (state_ == ST_MANUAL_OZONE) return true; // manual ozone requires vent after? keep fan on
  return fanRelay_;
}

bool FSM::relayOzone() const {
  return ozoneRelay_;
}

void FSM::enter(SystemState st) {
  state_ = st;
}

void FSM::setBacklight(bool on, uint32_t nowMs) {
  backlightOn_ = on;
  backlightChanged_ = nowMs;
}


