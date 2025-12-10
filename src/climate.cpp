#include "climate.h"

ClimateDecision ClimateController::evaluate(const FilteredReadings &f,
                           const SensorStatus &bme,
                           const SensorStatus &htu,
                           const SensorStatus &ds,
                           float targetTemp, float targetRh, uint32_t nowMs) {
  ClimateDecision d{};
  // При отказе любого датчика считаем состояние аварийным.
  if (!bme.ok || !htu.ok || !ds.ok) {
    d.sensorError = true;
    return d;
  }
  // Проверяем выгоду проветривания по абсолютной влажности.
  bool outdoorHelps = (!isnan(f.ahOut) && !isnan(f.ahIn)) &&
                      (f.ahOut + DEW_SAFETY_MARGIN < f.ahIn);
  // Риск конденсации по точке росы.
  bool condensation = (!isnan(f.dewIn) && !isnan(f.tin)) &&
                      (f.dewIn + DEW_SAFETY_MARGIN >= f.tin);
  d.condensationRisk = condensation;

  bool hot = (!isnan(f.tin) && f.tin > targetTemp + TEMP_HYST);
  bool humid = (!isnan(f.rin) && f.rin > targetRh + RH_HYST);

  // Запуск вентилятора — если превышение + улица суше + нет конденсата, не чаще 5 мин.
  bool canStart = (hot || humid) && outdoorHelps && !condensation;
  if (canStart) {
    if (nowMs - lastOnMs_ > 300000UL) { // 5 min anti-chatter
      fanLatched_ = true;
      lastOnMs_ = nowMs;
    }
  }

  // Условия остановки: гистерезис, конденсация, ухудшение улицы, ошибка.
  bool coolEnough = (!isnan(f.tin) && f.tin < targetTemp - TEMP_HYST);
  bool dryEnough = (!isnan(f.rin) && f.rin < targetRh - RH_HYST);
  bool stopConditions = condensation || !outdoorHelps || d.sensorError ||
                        (coolEnough && dryEnough);

  if (stopConditions) fanLatched_ = false;

  d.fanShouldRun = fanLatched_;
  return d;
}

void ClimateController::reset() {
  fanLatched_ = false;
}


