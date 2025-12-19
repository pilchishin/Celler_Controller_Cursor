#include <Arduino.h>
#include <avr/wdt.h>
#include "config.h"
#include "sensors.h"
#include "climate.h"
#include "ozone.h"
#include "fsm.h"
#include "eeprom.h"
#include "display.h"
#include "menu.h"
#include "timer.h"
#include "utils.h"

// Глобальные экземпляры подсистем.
Sensors sensors;
ClimateController climate;
OzoneController ozone;
FSM fsm;
EepromStore storage;
Display display;
Menu menu;

Config cfg;
Stats stats;
Scheduler sched;

uint32_t lastLoopMs = 0;
uint32_t lastRuntimeMs = 0;
uint32_t fanAccMs = 0;
uint32_t ozoneAccMs = 0;

// Кнопки подтянуты к VCC, активный уровень — LOW.
bool buttonPressed(uint8_t pin) {
  return digitalRead(pin) == LOW;
}

void setupPins() {
  // Настраиваем реле и кнопки.
  pinMode(PIN_SSR_FAN, OUTPUT);
  pinMode(PIN_SSR_OZONE, OUTPUT);
  digitalWrite(PIN_SSR_FAN, LOW);
  digitalWrite(PIN_SSR_OZONE, LOW);
  pinMode(PIN_BTN_MENU, INPUT_PULLUP);
  pinMode(PIN_BTN_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
}

void setup() {
  // Старт и включение watchdog 8s.
  wdt_enable(WDTO_8S);
  setupPins();
  storage.begin();
  cfg = storage.loadConfig();
  stats = storage.loadStats();

  sensors.begin(cfg.offsets);
  display.begin();
  fsm.begin();
  fsm.setConfig(&cfg);
  fsm.setStats(&stats);
  menu.begin(&display, &fsm, &storage, &cfg);
}

void handleButtons(uint32_t nowMs) {
  static uint32_t lastPress = 0;
  static uint8_t lastPin = 0;
  static uint32_t lastDebounce = 0;
  uint8_t pins[3] = {PIN_BTN_MENU, PIN_BTN_UP, PIN_BTN_DOWN};
  // Простая обработка удержания: пока держим, через MENU_LONGPRESS считаем long.
  for (uint8_t p : pins) {
    if (buttonPressed(p)) {
      if (lastPin != p) {
        // Новое нажатие: проверить debounce
        if (nowMs - lastDebounce >= MENU_DEBOUNCE_MS) {
          lastDebounce = nowMs;
          lastPin = p;
          lastPress = nowMs;
          menu.handleButton(p, false, nowMs);  // Новое нажатие, longP=false
          fsm.setBacklight(true, nowMs);
        }
      } else {
        // Повторное: проверить longPress
        bool longP = (nowMs - lastPress) > MENU_LONGPRESS_MS;
        menu.handleButton(p, longP, nowMs);
        fsm.setBacklight(true, nowMs);
      }
    } else if (lastPin == p) {
      // Отпустили кнопку
      lastPin = 0;
    }
  }
}

void loop() {
  uint32_t nowMs = millis();
  wdt_reset();
  if (lastRuntimeMs == 0) lastRuntimeMs = nowMs;

  if (due(nowMs, sched.lastSensors, SENSOR_PERIOD_MS)) {
    sensors.poll(nowMs, cfg.offsets);
  }

  FilteredReadings f = sensors.filtered();
  uint32_t epoch = nowEpoch();

  fsm.tick(nowMs, epoch, sensors, climate, ozone);

  // проверка запланированного запуска озона
  DateTime dt = rtc.now();
  if (cfg.ozone.enabled && dt.hour() == cfg.ozone.hour && dt.minute() == cfg.ozone.minute && dt.dayOfTheWeek() == cfg.ozone.weekday) {
    if (!ozone.status().running && ozone.status().state == OZ_IDLE && f.tout >= 0 && !fsm.backlightBlock()) {
      ozone.start(nowMs);
      storage.logEvent(EVT_OZONE_START, epoch);
    }
  }

  ozone.tick(nowMs, f, fsm.backlightBlock(), f.tout >= 0);

  // тик UI
  if (due(nowMs, sched.lastUi, UI_TICK_MS)) {
    menu.tick(nowMs, f, fsm.relayFan(), fsm.relayOzone());
  }

  // снимок статистики
  if (due(nowMs, sched.lastStats, STATS_SNAPSHOT_MS)) {
    storage.saveStats(stats);
  }

  // применение реле
  bool fanOn = fsm.relayFan();
  bool ozOn = fsm.relayOzone();
  digitalWrite(PIN_SSR_FAN, fanOn);
  digitalWrite(PIN_SSR_OZONE, ozOn);

  // накопление времени работы
  uint32_t delta = nowMs - lastRuntimeMs;
  lastRuntimeMs = nowMs;
  // Наращиваем минуты работы для статистики.
  if (fanOn) {
    fanAccMs += delta;
    while (fanAccMs >= 60000) {
      fanAccMs -= 60000;
      stats.fanMinutes++;
    }
  }
  if (ozOn) {
    ozoneAccMs += delta;
    while (ozoneAccMs >= 60000) {
      ozoneAccMs -= 60000;
      stats.ozoneMinutes++;
    }
  }

  handleButtons(nowMs);

  // темп цикла
  if (millis() - nowMs < 5) {
    delay(5);
  }
}


