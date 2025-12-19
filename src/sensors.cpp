#include "sensors.h"
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_HTU21DF.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RTClib.h>
#include "filters.h"

Adafruit_BME280 bme;
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
OneWire oneWire(PIN_ONEWIRE);
DallasTemperature ds(&oneWire);
RTC_DS3231 rtc;

bool Sensors::begin(const SensorOffsets &offsets) {
  bool ok = true;
  // Инициализация BME280 — основной датчик T/RH внутри.
  if (!bme.begin(I2C_BME280_ADDR)) ok = false, bmeOk_.ok = false;
  // Инициализация HTU21D — наружный датчик T/RH.
  if (!htu.begin()) ok = false, htuOk_.ok = false;
  // DS18B20 — резервный контроль температуры в подвале.
  ds.begin();
  // RTC DS3231 — контроль времени и расписания.
  if (!rtc.begin()) {
    rtcOk_.ok = false;
    ok = false;
  } else {
    if (rtc.lostPower()) rtcOk_.ok = false, ok = false;
  }
  // Первичное чтение для заполнения фильтров.
  poll(0, offsets); // начальное чтение
  return ok;
}

bool Sensors::poll(uint32_t nowMs, const SensorOffsets &offsets) {
  // Опрос каждые 10 секунд.
  if (nowMs - lastPollMs_ < SENSOR_PERIOD_MS) return true;
  lastPollMs_ = nowMs;
  bool ok = true;
  ok &= readBme(offsets);
  ok &= readHtu(offsets);
  ok &= readDs(offsets);
  ok &= checkRtc();
  updateDerived();
  return ok;
}

bool Sensors::readBme(const SensorOffsets &offsets) {
  float t = bme.readTemperature();
  float h = bme.readHumidity();
  // Проверка на валидность — если NAN, накапливаем ошибки.
  if (isnan(t) || isnan(h)) {
    if (++bmeOk_.failures >= 3) bmeOk_.ok = false;
    return false;
  }
  bmeOk_.failures = 0;
  bmeOk_.ok = true;
  raw_.tin = t + offsets.bmeTemp;
  raw_.rin = clampValue(h + offsets.bmeRh, 0.0f, 100.0f);
  return true;
}

bool Sensors::readHtu(const SensorOffsets &offsets) {
  float t = htu.readTemperature();
  float h = htu.readHumidity();
  // HTU21D — наружный воздух для оценки выгоды проветривания.
  if (isnan(t) || isnan(h)) {
    if (++htuOk_.failures >= 3) htuOk_.ok = false;
    return false;
  }
  htuOk_.failures = 0;
  htuOk_.ok = true;
  raw_.tout = t + offsets.htuTemp;
  raw_.rout = clampValue(h + offsets.htuRh, 0.0f, 100.0f);
  return true;
}

bool Sensors::readDs(const SensorOffsets &offsets) {
  ds.requestTemperatures();
  float t = ds.getTempCByIndex(0);
  // DS18B20: считаем ошибкой экстремальные/NaN значения.
  if (t <= -127 || t >= 125 || isnan(t)) {
    if (++dsOk_.failures >= 3) dsOk_.ok = false;
    return false;
  }
  dsOk_.failures = 0;
  dsOk_.ok = true;
  raw_.tin = (isnan(raw_.tin) ? t : raw_.tin); // BME основной, DS резервный для безопасности
  raw_.tin += offsets.ds18b20;
  return true;
}

bool Sensors::checkRtc() {
  // Проверяем доступность RTC и отсутствие сброса питания.
  if (!rtc.begin()) {
    if (++rtcOk_.failures >= 3) rtcOk_.ok = false;
    return false;
  }
  if (rtc.lostPower()) {
    if (++rtcOk_.failures >= 3) rtcOk_.ok = false;
    return false;
  }
  rtcOk_.ok = true;
  rtcOk_.failures = 0;
  return true;
}

void Sensors::updateDerived() {
  // Фильтрация: медиана окна 3 + EMA для каждой метрики.
  if (!isnan(raw_.tin)) {
    medTin_.push(raw_.tin);
    float m = medTin_.median();
    filt_.tin = emaTin_.update(m);
  }
  if (!isnan(raw_.rin)) {
    medRin_.push(raw_.rin);
    filt_.rin = emaRin_.update(medRin_.median());
  }
  if (!isnan(raw_.tout)) {
    medTout_.push(raw_.tout);
    filt_.tout = emaTout_.update(medTout_.median());
  }
  if (!isnan(raw_.rout)) {
    medRout_.push(raw_.rout);
    filt_.rout = emaRout_.update(medRout_.median());
  }
  if (!isnan(filt_.tin) && !isnan(filt_.rin)) {
    // Точка росы и абсолютная влажность по спецификации.
    filt_.dewIn = computeDew(filt_.tin, filt_.rin);
    filt_.ahIn = computeAH(filt_.tin, filt_.rin);
  }
  if (!isnan(filt_.tout) && !isnan(filt_.rout)) {
    filt_.dewOut = computeDew(filt_.tout, filt_.rout);
    filt_.ahOut = computeAH(filt_.tout, filt_.rout);
  }
}

float computeDew(float t, float rh) {
  float gamma = log(rh / 100.0f) + (17.62f * t) / (243.12f + t);
  return (243.5f * gamma) / (17.62f - gamma);
}

float computeAH(float t, float rh) {
  float svp = 6.112f * exp((17.62f * t) / (243.12f + t));
  float vp = (rh / 100.0f) * svp;
  return 216.7f * vp / (t + 273.15f);
}


