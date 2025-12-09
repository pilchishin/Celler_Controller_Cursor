#pragma once

#include <Arduino.h>

// Универсальный пайплайн: окно медианы + EMA для одной величины.
struct FilterPipeline {
  float medianValues[3]{NAN, NAN, NAN};
  float ema{NAN};
  float alpha{0.2f};
  uint8_t idx{0};
};

// Обновить медиану (окно 3).
float applyMedian(FilterPipeline &f, float v);
// Обновить EMA.
float applyEma(FilterPipeline &f, float v);


