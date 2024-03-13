
void printMainWindow() { //функция для вывода на экран меню текущих значений
    lcd.setCursor(0, 0); lcd.print("t:"); lcd.print(TE1); lcd.print("C"); //Температура текущая 
    lcd.setCursor(8, 0); lcd.print("T:"); lcd.print(timeCyclMins); lcd.print(":"); lcd.print(timeCyclSecs); // Время общее. Глобальный таймер
    lcd.setCursor(0, 1); lcd.print("t:"); lcd.print(tempValue); lcd.print("C"); //Температура цыкла 
    lcd.setCursor(8, 1); lcd.print("T:"); lcd.print(timesValue); lcd.print(" min"); //Время цыкла
}

int settingsWorkingWindow[][]  = {  //Перечень параметров для вывода информации на/в рабочее окно
    {line_11, line_12},             // "t:", TE1 - Температура текущая. "T:", timeCyclMins, timeCyclSecs - Время общее. Таймер процесса
    {line_21, line_22},             // "t:", tempValue - Температура цыкла, "T:" timesValue - Время цыкла
    {line_31, line_32},             // 
    {line_41, line_42}              // Глобальный таймер
};


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