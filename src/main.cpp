#include <Arduino.h>

// - ПАРАМЕТРЫ МЕНЮ НАЧАЛО

#define LINES 2   // количество строк дисплея
#define SETTINGS_AMOUNT 13  // количество настроек
#define SETTINGS_SETTING 12  // количество настроек
#define SETTINGS_MENU 3 // количество настроек

#define LEDSTART 7 // пин индикация старта красный светодиод

bool controlState = false;  // клик

// пины энкодера
#define CLK 2
#define DT 3
#define SW 4

enum class Menu {  MainMenu,  MainWindow,  SettingsValue,  StartStopSettings }; //Создаём классы

#include <GyverEncoder.h>
Encoder enc1(CLK, DT, SW);  // для работы c кнопкой

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); // адрес 0x27 или 0x3f

int vals[SETTINGS_SETTING];  // массив параметров
int8_t arrowPos = 0;
int8_t entVal = 0;
int8_t screenPos = 0; // номер "экрана"
int8_t dataEntry = 0; // переменная для хранения данных в диапахоне от 0 до 60 и от 0 до 100

bool settingPosition = false;  // переменная для ввода хгачений в настройка. Это флаг миняющий сой состояние при клике энкодера
bool checkTE1 = 0;

Menu menu;
// названия параметров
String settingsValue[]  = { //Перечень окна с натройками
  "Time-1", //Время 
  "Temp-1", //Температура
  "Time-2",
  "Temp-2",
  "Time-3",
  "Temp-3",
  "Time-4",
  "Temp-4",
  "Time-5",
  "Temp-5",
  "Time-6",
  "Temp-6",
  "Exit",
}; //

bool StrStp = 0; // Состояние переменной инициализирует работу программы

String settingsMainMenu[]  = { // Главное окно меню
  "Setting",
  "Window",
  "Stop",
};

String settingsStart[]  = { //Меню запуска програмы
  "Stop",
  "Start",
};

// - ПАРАМЕТРЫ ТАЙМЕРА НАЧАЛО
#include "GyverTimer.h" 

GTimer TimerBRW_1(MS);
GTimer TimerBRW_2(MS);
GTimer TimerBRW_3(MS);

uint32_t period = 1000;

bool flagCycl = false;
bool timerStart = true;
int valsIndex = 0, k = 0, ai = 0, ti = 0, fulTim = 0, summ_arr = 0;
uint32_t timer_brw1, timer_brw2, timer_brw3;
//Переменные для глобального таймера
uint32_t totalMills; //Милесек
uint32_t totalSec; // секунды
//Переменные для цикла таймера
uint32_t totalCyclMills = 0; //Милесек
uint32_t totalCyclSec = 0; // секунды

uint32_t totalCurrentSec = 0; // передаём в секундах  
uint32_t currentMillis = 0;

int timeHours = 0;; // часы
int timeMins = 0;  // минуты
int timeSecs = 0;  // секунды

int timeCyclHours = 0;; // часы
int timeCyclMins = 0;  // минуты
int timeCyclSecs = 0;  // секунды

// - ПАРАМЕТРЫ ПИД НАЧАЛО
#include <GyverPID.h>
//
//16:52:14.353 -> result: PI p: 7.72	PI i: 0.41	PID p: 21.56	PID i: 0.25	PID d: 64.99
GyverPID pid(21.56, 0.25, 0);
int PIDperiod = 500;

#define RELE_5 5
// - ПАРАМЕТРЫ ПИД КОНЕЦ

// - ПАРАМЕТРЫ датчика температуры
#include <microDS18B20.h>
MicroDS18B20<6> sensTE;

int temp1, TE1; // переменная для хранения температуры
int tempValue = 0; // переменная для хранения температуры 
int timesValue = 0;

void f_TE1() { //Функия чтения температуры
  static uint32_t tmr2;
    if (millis() - tmr2 >= PIDperiod) {
      tmr2 = millis();    
    
      sensTE.readTemp();
      TE1 = sensTE.getTempInt(); 
      sensTE.requestTemp();

    //return temp1;
  }
}

void f_timer(); //Объявление функции

void f_timer()
{
  totalSec = millis() / 1000ul;
  timeHours = (totalSec / 3600ul);        // часы
  timeMins = (totalSec % 3600ul) / 60ul;  // минуты
  timeSecs = (totalSec % 3600ul) % 60ul;  // секунды
}



void printMainWindow(); //Объявление функции

void printMainWindow(){ //функция для вывода на экран меню текущих значений
  
    lcd.setCursor(0, 0); lcd.print("t:"); lcd.print(TE1); lcd.print("C"); //Температура текущая 
    lcd.setCursor(8, 0); lcd.print("T:"); lcd.print(timeCyclMins); lcd.print(":"); lcd.print(timeCyclSecs); // Время общее. Глобальный таймер
    lcd.setCursor(0, 1); lcd.print("t:"); lcd.print(tempValue); lcd.print("C"); //Температура цыкла 
    lcd.setCursor(8, 1); lcd.print("T:"); lcd.print(timesValue); lcd.print(" min"); //Время цыкла
    
}

void printSettingsValue();

void printSettingsValue() {  //Функция для вывода на экран меню настроек
  lcd.clear();  
  screenPos = arrowPos / LINES;   // ищем номер экрана (0..3 - 0, 4..7 - 1)

    for (byte i = 0; i < LINES; i++) {  // для всех строк
      
      lcd.setCursor(0, i); // курсор в начало  
      
      // если курсор находится на выбранной строке
      if (arrowPos == LINES * screenPos + i) lcd.write(126);  // рисуем стрелку
      else lcd.write(32);     // рисуем пробел

      // если пункты меню закончились, покидаем цикл for
      if (LINES * screenPos + i == SETTINGS_AMOUNT) break;

      // выводим имя и значение пункта меню
      lcd.print(settingsValue[LINES * screenPos + i]);
      if (arrowPos < SETTINGS_SETTING) {
        lcd.print(": ");
        lcd.print(vals[LINES * screenPos + i]);
      }

    }
  }

void printMainMenu();

void printMainMenu(){ //Функция для вывода на экран главное меню
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

void f_pid();

void f_pid(){ //Функция ПИД
  
  static uint32_t tmr3;
  if (millis() - tmr3 >= PIDperiod) {
    tmr3 = millis();
    pid.input = TE1;   // сообщаем регулятору текущую температуру
    pid.getResult();
    analogWrite(RELE_5, pid.output);    
    //ploter
    //Serial.print("TE "); Serial.print(pid.input); Serial.print(' ');
    //Serial.print("SET "); Serial.println(pid.setpoint);
    //Serial.print("pid.output "); Serial.println(pid.output);
  }

}

void setup() {
  Serial.begin(9600); //Волшебная цифра 
  enc1.setType(TYPE2); //Тип энкодера
  enc1.setFastTimeout(100);

  lcd.init();
  lcd.backlight();
  
  Serial.println("Start");
  
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(" Brewery V 0.6");
  lcd.setCursor(0, 1); lcd.print("");

  TimerBRW_2.setInterval(100000); //Таймер для вывода сообщения в консоль  
  
  sensTE.requestTemp();     // запрос температуры
  delay(500);
  f_TE1();

  pinMode(RELE_5, OUTPUT);
  pinMode(LEDSTART, OUTPUT);

  pid.setDirection(NORMAL); // направление регулирования (NORMAL/REVERSE). ПО УМОЛЧАНИЮ СТОИТ NORMAL - нагрев
  pid.setLimits(0, 255);    // пределы (ставим для 8 битного ШИМ). ПО УМОЛЧАНИЮ СТОЯТ 0 И 255
  
  delay(3000);

  lcd.clear();
  lcd.setCursor(4, 0); lcd.print("TE ");
  lcd.setCursor(8, 0); lcd.print(TE1);

  delay(2000);
  //menu = Menu::MainMenu;
  printMainMenu();
}

void loop() {

  f_timer(); //Запуск глобального таймера
  f_TE1(); //переменная для хранения температуры
  static int timeSecs_tmp = 0;
  enc1.tick();
  
  switch (menu) {

    case Menu::MainMenu: //Главное Меню
      if (enc1.isTurn()) {
        int increment = 0;  // локальная переменная направления
        // получаем направление   
        if (enc1.isRight()) increment = 1;
        if (enc1.isLeft()) increment = -1;
        arrowPos += increment;  // двигаем курсор  
        arrowPos = constrain(arrowPos, 0, SETTINGS_MENU - 1); // ограничиваем

        increment = 0;  // обнуляем инкремент

        printMainMenu(); //Выводим на экран соответсвующие меню
        Serial.println(arrowPos);
        
      }
      if (enc1.isClick()) {
        if (0 == arrowPos) {
          lcd.clear();
          printSettingsValue();
          menu = Menu::SettingsValue;
        }
        if (1 == arrowPos) {
          lcd.clear();
          printMainWindow();
          menu = Menu::MainWindow;
        }
        if (2 == arrowPos) {
          StrStp = !StrStp;
				  if (StrStp == 1){
            settingsMainMenu[2] = "Start";
            valsIndex == 0;
          }
          else {
            settingsMainMenu[2] = "Stop";
          }
          lcd.clear();
          printMainMenu();

        }
      }

      break;

    case Menu::SettingsValue: //Меню настроек

      if (!settingPosition) {
        if (enc1.isTurn()) {
          int increment = 0;  // локальная переменная направления
        
          // получаем направление   
          if (enc1.isRight()) increment = 1;
          if (enc1.isLeft()) increment = -1;
          arrowPos += increment;  // двигаем курсор  
          arrowPos = constrain(arrowPos, 0, SETTINGS_AMOUNT - 1); // ограничиваем

          increment = 0;  // обнуляем инкремент
      
          printSettingsValue();
        }
      }
      else if (settingPosition) {
        if (enc1.isTurn()) {
          int increment = 0;  // локальная переменная направления
          if (arrowPos < SETTINGS_SETTING) {
            
            if (enc1.isRight()) increment = 1;
            if (enc1.isFastR()) increment = 5;
            if (enc1.isLeft()) increment = -1;
            if (enc1.isFastL()) increment = -5;

            vals[arrowPos] += increment;
            if (arrowPos % 2 == 0) {
              dataEntry = constrain(vals[arrowPos], 0, 60); // Ограничени для времени
              vals[arrowPos] = dataEntry;
            }
            else {
              dataEntry = constrain(vals[arrowPos], 0, 100); // Ограничени для температуры
              vals[arrowPos] = dataEntry;
            }
          }
          printSettingsValue();
        }
      }
      if (enc1.isClick()){
        settingPosition = !settingPosition;
        Serial.println("settingPosition -> "); Serial.println(settingPosition);
       
        if (arrowPos == SETTINGS_AMOUNT-1) {
          arrowPos = 0;
          lcd.clear();
          printMainMenu(); 
          Serial.println("SettingsValue");
          menu = Menu::MainMenu;
        }
      }

      break;

    case Menu::MainWindow:
      if(timeSecs_tmp != timeSecs) {
        timeSecs_tmp = timeSecs;
        printMainWindow();
      }
      if (enc1.isClick()){ //По нажатию проваливаемся в соответсвующее меню
        lcd.clear();
        Serial.println("MainWindow");
        printMainMenu();  
        menu = Menu::MainMenu;
      }     
      break;   

    case Menu::StartStopSettings:
      break;
  }

  if (StrStp) {

    digitalWrite(LEDSTART, HIGH);
  //---------------------------------------
    timesValue = vals[valsIndex]; //Время паузы цыкла
    tempValue = vals[valsIndex+1]; //Температура паузы цыкла 

    if (TE1 <= tempValue - 1) { 
      pid.setpoint = tempValue; //Передаём температуру в пид регулятор до момента TE - 1
      f_pid();
    }
    else {
      pid.setpoint = 0; //Передаём температуру в пид регулятор 0, для избежания перерегулирования
      f_pid();
    }

    //bool flagCycl = false;
    //bool timerStart = true;

    if (TE1 >= tempValue - 1) {  // Это условие точно нужно?
      
      if (timerStart) {
       
        totalCurrentSec = (timesValue * 60ul) + 1 ; // передаём в секундах  
        currentMillis = millis();
        
        timerStart = false;
        flagCycl = true;
      }

      if (flagCycl) {
        if (millis() >= (timesValue * 60ul * 1000ul) + currentMillis) {
          valsIndex += 2;
          timerStart = true;
          flagCycl = false;
        }
        if (valsIndex > SETTINGS_SETTING) {
          flagCycl = false;
          timerStart = true;
          StrStp = false;
          settingsMainMenu[2] = "Stop";
          valsIndex = 0;

        }
        static uint32_t tmr4;
        if (millis() - tmr4 >= 1000) {
          tmr4 = millis();
          totalCurrentSec -= 1;
          timeCyclHours = (totalCurrentSec / 3600ul);        // часы
          timeCyclMins = (totalCurrentSec % 3600ul) / 60ul;  // минуты
          timeCyclSecs = (totalCurrentSec % 3600ul) % 60ul;  // секунды
        }
      }
    }
  }
  else {
    digitalWrite(LEDSTART, LOW);
    
    pid.setpoint = 0;
    f_pid();
  }
//---------------------------------------
}