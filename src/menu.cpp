#include "menu.h"
#include "utils.h"

void Menu::begin(Display *disp, FSM *fsm, EepromStore *store, Config *cfg) {
  disp_ = disp;
  fsm_ = fsm;
  store_ = store;
  cfg_ = cfg;
  disp_->begin();
}

void Menu::tick(uint32_t nowMs, const FilteredReadings &f, bool fanOn, bool ozOn) {
  // Поддерживаем подсветку в соответствии с блокировкой FSM.
  backlight_ = fsm_->backlightBlock();
  disp_->backlight(backlight_);
  render(f, fanOn, ozOn);
}

void Menu::handleButton(uint8_t btn, bool longPress, uint32_t nowMs) {
  Serial.println("Processing button " + String(btn) + " longPress: " + String(longPress) + " at " + String(nowMs));

  backlight_ = true;
  // Простая матрица кнопок: Menu — перелистывание, long Menu — сброс статистики.
  if (btn == PIN_BTN_MENU) {
    if (longPress) {
      // сервис: сброс статистики
      Stats st{};
      store_->saveStats(st);
    } else {
      nextPage();
    }
  } else if (btn == PIN_BTN_UP) {
    // UP: настройки RH или запуск ручного режима.
    if (page_ == PAGE_SETTINGS) {
      cfg_->targetRh = clampValue(cfg_->targetRh + 0.5f, 50.0f, 95.0f);
      applyConfigChange();
    } else if (page_ == PAGE_MANUAL_FAN) {
      fsm_->requestManualFan(15, nowMs);
    } else if (page_ == PAGE_MANUAL_OZONE && longPress) {
      fsm_->requestManualOzone(15, nowMs);
    }
  } else if (btn == PIN_BTN_DOWN) {
    // DOWN: настройка целевой температуры.
    if (page_ == PAGE_SETTINGS) {
      cfg_->targetTemp = clampValue(cfg_->targetTemp - 0.1f, 2.0f, 8.0f);
      applyConfigChange();
    }
  }
}

void Menu::applyConfigChange() {
  // Немедленно сохраняем конфигурацию при изменении.
  store_->saveConfig(*cfg_);
}

void Menu::render(const FilteredReadings &f, bool fanOn, bool ozOn) {
  // Статус перелистывается автоматически; остальные страницы — заглушки.
  if (page_ == PAGE_STATUS) {
    disp_->showStatusPage(statusPage_, f, fanOn, ozOn);
    statusPage_ ^= 1;
  } else if (page_ == PAGE_MANUAL_FAN) {
    disp_->showError("Manual Fan");
  } else if (page_ == PAGE_MANUAL_OZONE) {
    disp_->showError("Manual Ozone");
  } else if (page_ == PAGE_SETTINGS) {
    // простое представление настроек
    disp_->showError("Settings");
  }
}

void Menu::nextPage() {
  page_ = static_cast<MenuPage>((page_ + 1) % PAGE_MAX);
}

void Menu::prevPage() {
  if (page_ == 0) page_ = static_cast<MenuPage>(PAGE_MAX - 1);
  else page_ = static_cast<MenuPage>(page_ - 1);
}


