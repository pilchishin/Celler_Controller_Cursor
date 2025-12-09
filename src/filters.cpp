#include "filters.h"
#include <math.h>

// Добавляем значение в окно и возвращаем медиану.
float applyMedian(FilterPipeline &f, float v) {
  f.medianValues[f.idx % 3] = v;
  f.idx++;
  float a = f.medianValues[0];
  float b = f.medianValues[1];
  float c = f.medianValues[2];
  float vals[3] = {a, b, c};
  for (int i = 0; i < 3; ++i) {
    for (int j = i + 1; j < 3; ++j) {
      if (vals[j] < vals[i]) {
        float t = vals[i];
        vals[i] = vals[j];
        vals[j] = t;
      }
    }
  }
  return vals[1];
}

// Экспоненциальное сглаживание с коэффициентом alpha.
float applyEma(FilterPipeline &f, float v) {
  if (isnan(f.ema)) f.ema = v;
  else f.ema = f.ema + f.alpha * (v - f.ema);
  return f.ema;
}


