#pragma once

#include "sensors.h"
#include "config.h"

// Результат оценки микроклимата.
struct ClimateDecision {
  bool fanShouldRun{false};
  bool condensationRisk{false};
  bool sensorError{false};
};

// Контроллер вентканала по правилам ТЗ (вариант C).
class ClimateController {
public:
  ClimateDecision evaluate(const FilteredReadings &f, const SensorStatus &bme,
                           const SensorStatus &htu, const SensorStatus &ds,
                           float targetTemp, float targetRh, uint32_t nowMs);
  void reset();
private:
  uint32_t lastOnMs_{0};
  bool fanLatched_{false};
};


