  static uint32_t tmr3;
  if (millis() - tmr3 >= PIDperiod) {


// Исправление функции глобального таймера
void f_timer(){ 
  if (millis() - totalSecTmr >= 1000) {
    static uint32_t totalSec; // удалить глобальную переменную
    static uint32_t totalSecTmr;
    totalSec = millis() / 1000ul;
    timeHours = (totalSec / 3600ul);        // часы глобальная переменная
    timeMins = (totalSec % 3600ul) / 60ul;  // минуты глобальная переменная
    timeSecs = (totalSec % 3600ul) % 60ul;  // секунды глобальная переменная
  }
}

printWorkingWindow();
void printWorkingWindow(){ //Функция для вывода на экран главное меню
  lcd.clear();  
  screenPos = arrowPos / LINES;   // ищем номер экрана (0..3 - 0, 4..7 - 1)

  for (byte i = 0; i < LINES; i++) {  // для всех строк
    lcd.setCursor(0, i);              // курсор в начало
    // если курсор находится на выбранной строке
    if (arrowPos == LINES * screenPos + i) lcd.write(126);  // рисуем стрелку
    else lcd.write(32);     // рисуем пробел
    // если пункты меню закончились, покидаем цикл for
    if (LINES * screenPos + i == SETTINGS_MENU) break;
    // выводим имя и значение пункта меню
    lcd.print(settingsMainMenu[LINES * screenPos + i]);
  }
}