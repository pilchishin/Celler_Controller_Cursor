#include "utils.h"
#include <RTClib.h>

extern RTC_DS3231 rtc;

// Итеративное обновление CRC16 (Modbus полином 0xA001).
uint16_t crc16_update(uint16_t crc, uint8_t data) {
  crc ^= data;
  for (uint8_t i = 0; i < 8; i++) {
    if (crc & 1)
      crc = (crc >> 1) ^ 0xA001;
    else
      crc >>= 1;
  }
  return crc;
}

// CRC16 для буфера произвольной длины.
uint16_t crc16_bytes(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; ++i) {
    crc = crc16_update(crc, data[i]);
  }
  return crc;
}

// Добавляем новое значение в окно медианы.
void RunningMedian3::push(float v) {
  c = b;
  b = a;
  a = v;
}

// Возвращаем средний элемент отсортированной тройки.
float RunningMedian3::median() const {
  float vals[3] = {a, b, c};
  // Простая сортировка для трёх значений
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

// Экспоненциальное сглаживание.
float EmaFilter::update(float v) {
  if (isnan(value)) {
    value = v;
  } else {
    value = value + alpha * (v - value);
  }
  return value;
}

// Текущее время unix с RTC (0 если недоступен).
uint32_t nowEpoch() {
  if (!rtc.begin()) return 0;
  DateTime now = rtc.now();
  return now.unixtime();
}

// Проверка корректности RTC и батареи.
bool rtcValid() {
  if (!rtc.begin()) return false;
  return rtc.lostPower() == false;
}


