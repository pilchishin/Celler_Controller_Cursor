#pragma once

#include <Arduino.h>
#include "config.h"
#include <RTClib.h>
#include "utils.h"

// Флаг и счётчик отказов для каждого датчика.
struct SensorStatus {
  bool ok{true};
  uint8_t failures{0};
};

// Неотфильтрованные показания.
struct RawReadings {
  float tin{NAN};
  float rin{NAN};
  float tout{NAN};
  float rout{NAN};
};

// Отфильтрованные + производные показатели.
struct FilteredReadings {
  float tin{NAN};
  float rin{NAN};
  float tout{NAN};
  float rout{NAN};
  float dewIn{NAN};
  float dewOut{NAN};
  float ahIn{NAN};
  float ahOut{NAN};
};

// Класс для инициализации и периодического опроса всех датчиков.
class Sensors {
public:
  bool begin(const SensorOffsets &offsets);
  bool poll(uint32_t nowMs, const SensorOffsets &offsets);

  RawReadings raw() const { return raw_; }
  FilteredReadings filtered() const { return filt_; }

  SensorStatus bmeStatus() const { return bmeOk_; }
  SensorStatus htuStatus() const { return htuOk_; }
  SensorStatus dsStatus() const { return dsOk_; }
  SensorStatus rtcStatus() const { return rtcOk_; }

private:
  void updateDerived();
  bool readBme(const SensorOffsets &offsets);
  bool readHtu(const SensorOffsets &offsets);
  bool readDs(const SensorOffsets &offsets);
  bool checkRtc();

  RawReadings raw_{};
  FilteredReadings filt_{};

  RunningMedian3 medTin_, medRin_, medTout_, medRout_;
  EmaFilter emaTin_{}, emaRin_{}, emaTout_{}, emaRout_{};

  uint32_t lastPollMs_{0};
  SensorStatus bmeOk_{}, htuOk_{}, dsOk_{}, rtcOk_{};
};

float computeDew(float t, float rh);
float computeAH(float t, float rh);

extern RTC_DS3231 rtc;


