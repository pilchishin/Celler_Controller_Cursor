#pragma once

#include <Arduino.h>

// CRC16 для контроля целостности конфигурации/статистики.
uint16_t crc16_update(uint16_t crc, uint8_t data);
uint16_t crc16_bytes(const uint8_t *data, size_t len);

// Ограничение значения в диапазоне.
template <typename T>
T clampValue(T v, T lo, T hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// Скользящее окно для медианы из 3 значений.
struct RunningMedian3 {
  float a{NAN}, b{NAN}, c{NAN};
  void push(float v);
  float median() const;
};

// EMA-фильтр для плавного сглаживания.
struct EmaFilter {
  float value{NAN};
  float alpha{0.2f};
  float update(float v);
};

// Вспомогательные функции RTC.
uint32_t nowEpoch();
bool rtcValid();


