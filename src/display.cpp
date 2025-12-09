#include "display.h"

bool Display::begin() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Cellar Ctrl"); // приветствие при старте
  return true;
}

void Display::backlight(bool on) {
  if (on) lcd.backlight();
  else lcd.noBacklight();
}

void Display::showStatusPage(uint8_t page, const FilteredReadings &f, bool fanOn, bool ozOn) {
  // Две страницы: 0 — T/RH, 1 — AH и состояние реле.
  lcd.clear();
  lcd.setCursor(0, 0);
  if (page == 0) {
    lcd.print("Tin ");
    lcd.print(f.tin, 1);
    lcd.print("C ");
    lcd.print("RH ");
    lcd.print(f.rin, 0);
    lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("Tout");
    lcd.print(f.tout, 1);
    lcd.print("C ");
    lcd.print("RH ");
    lcd.print(f.rout, 0);
    lcd.print("%");
  } else {
    lcd.print("AHin ");
    lcd.print(f.ahIn, 1);
    lcd.print("g");
    lcd.setCursor(0, 1);
    lcd.print("Fan:");
    lcd.print(fanOn ? "ON " : "OFF");
    lcd.print(" Oz:");
    lcd.print(ozOn ? "ON" : "OFF");
  }
}

void Display::showError(const char *msg) {
  lcd.clear();
  lcd.print("ERROR:");
  lcd.setCursor(0, 1);
  lcd.print(msg);
}


