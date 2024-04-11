#include <Arduino.h>

#define LINES 2   // количество строк дисплея
#define SETTINGS_AMOUNT 13  // количество настроек
#define SETTINGS_SETTING 12  // количество настроек
#define SETTINGS_MENU 3 // количество настроек

#define LEDSTART 7 // пин индикация старта красный светодиод
#define FANACTIVATION  10 // пин включения вентельятора (реле)

bool controlState = false;  // клик

// пины энкодера
#define CLK 2 // CLK (тактовые выводы энкодера)
#define DT 3 //DT (тактовые выводы энкодера)
#define SW 4 //SW (вывод кнопки) 

enum class Menu {MainMenu,  MainWindow,  SettingsValue,  StartStopSettings}; //Создаём классы

#include <GyverEncoder.h>
Encoder enc1(CLK, DT, SW);  // для работы c кнопкой

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); // адрес 0x27 или 0x3f

int vals[SETTINGS_SETTING];  // массив параметров
int8_t arrowPos = 0;
int8_t entVal = 0;
int8_t screenPos = 0; // номер "экрана"
int8_t dataEntry = 0; // переменная для хранения данных в диапахоне от 0 до 60 и от 0 до 100

bool settingPosition = false;  // переменная для ввода значений в настройка. Это флаг миняющий свой состояние при клике энкодера

Menu menu;
// названия параметров
String settingsValue[]  = { //Перечень окна с натройками
  "Time-1", //Время           0
  "Temp-1", //Температура     1
  "Time-2", //                2
  "Temp-2", //                3
  "Time-3", //                4
  "Temp-3", //                5
  "Time-4", //                6
  "Temp-4", //                7
  "Time-5", //                8
  "Temp-5", //                9
  "Time-6", //                10
  "Temp-6", //                11
  "Exit",   //                12
};

bool StrStp = false; // Состояние переменной инициализирует работу программы
bool startingCycle = false;  // запуск циклов

String settingsMainMenu[]  = { // Главное окно меню
  "Setpoints",
  "Window",
  "Stop",
};

String settingsStart[]  = { //Меню запуска програмы
  "Stop",
  "Start",
};

// - ПАРАМЕТРЫ ТАЙМЕРА НАЧАЛО

uint32_t period = 1000;

int valsIndex = 0;
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
int PIDperiod = 1000;

#define RELE_5 5 //Реле управления нагрузкой
// - ПАРАМЕТРЫ ПИД КОНЕЦ

// - ПАРАМЕТРЫ датчика температуры
#include <microDS18B20.h>
MicroDS18B20<6> sensTE1; // 6pin (пин) для 
MicroDS18B20<9> sensTE2; // 9pin (пин) для 

int temp1, TE1, temp2, TE2; // переменная для хранения температуры te1 для ТЭН te2 для куллера
int tempValue = 0; // переменная для запаси температуры из массива Value
int timesValue = 0; // переменная для запаси времени из массива Value
int fanActivationTemp = 35; // Максимальная температура внутри корпуса

void f_TE1(); //Функия чтения температуры
void f_TE1() { 
  static uint32_t tmr2;
  if (millis() - tmr2 >= PIDperiod) {
    tmr2 = millis();    
    
    sensTE1.readTemp();
    TE1 = sensTE1.getTempInt(); //чтение температуры с датчика для ТЭН
    sensTE1.requestTemp();

    sensTE2.readTemp();
    TE2 = sensTE2.getTempInt(); //чтение температуры с датчика для куллера
    sensTE2.requestTemp();

    //return temp1;
  }
}

void f_timer(); //Глобальный таймер
void f_timer() {
  totalSec = millis() / 1000ul;
  timeHours = (totalSec / 3600ul);        // часы
  timeMins = (totalSec % 3600ul) / 60ul;  // минуты
  timeSecs = (totalSec % 3600ul) % 60ul;  // секунды
}

void printMainWindow(); //функция для вывода на экран меню текущих значений
void printMainWindow() { 
 static uint32_t tmr9;
 if (millis() - tmr9 >= PIDperiod*5) {
    tmr9 = millis();
    
 }
  lcd.setCursor(0, 0); lcd.print("t:"); //Температура текущая 
  if(TE1 < 10) lcd.print("0"); 
  lcd.print(TE1); lcd.print("C");

  lcd.setCursor(8, 0); lcd.print("T:"); lcd.print(timeCyclMins);// Вывод секунд с добавлением ведущего нуля, если секунды меньше 10
  lcd.print(":"); 
  if(timeCyclSecs < 10) lcd.print("0"); 
  lcd.print(timeCyclSecs); 
  
  lcd.setCursor(0, 1); lcd.print("t:"); //Температура цыкла 
  if(tempValue < 10) lcd.print("0"); 
  lcd.print(tempValue); lcd.print("C");

  lcd.setCursor(8, 1); lcd.print("T:"); lcd.print(timesValue); lcd.print(" min"); //Время цыкла
}


void printSettingsValue();
void printSettingsValue() {  //Функция для вывода на экран меню настроек
  lcd.clear();  
  screenPos = arrowPos / LINES;   // ищем номер экрана (0..3 - 0, 4..7 - 1)

    for (byte i = 0; i < LINES; i++) {  // для всех строк

      lcd.setCursor(0, i); // курсор в начало  
      
      // если курсор находится на выбранной строке 
      if (arrowPos == LINES * screenPos + i) {
        if (settingPosition and arrowPos != SETTINGS_AMOUNT-1) {
          lcd.write(62); // рисуем стрелку >
        }
        else {
          lcd.write(126); // рисуем стрелку ->
        }
      }  
      else lcd.write(32); // рисуем пробел

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

//Функция для вывода на экран главное меню
void printMainMenu();
void printMainMenu() { 
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

//Функция ПИД
void f_pid();
void f_pid(){ 
  static uint32_t tmr3;
  if (millis() - tmr3 >= PIDperiod) {
    tmr3 = millis();
    pid.input = TE1;   // сообщаем регулятору текущую температуру
    pid.getResult();
    analogWrite(RELE_5, pid.output);    
  }
}

//функция для охлаждения корпуса, управление кулером // FANACTIVATION
void cooling_case();
void cooling_case(){
  static uint32_t tmr5;
  if (millis() - tmr5 >= PIDperiod * 2) {
    tmr5 = millis();
    if(TE2 > fanActivationTemp ){
      digitalWrite(FANACTIVATION, HIGH);
    }
    else if (TE2 <= fanActivationTemp - 5 ) {
      digitalWrite(FANACTIVATION, LOW);
    } 
  }
}

//функция световой индикации
void LED_indication();
void LED_indication(){
  //todo
}

//Проверка заполения 
bool sum_setting();
bool sum_setting(){
  int sum_vals = 0;
  for (int i = 0; i < SETTINGS_SETTING; i++) {
    sum_vals += vals[i];
  }
  
  //Serial.println("sum_vals -> "); Serial.println(sum_vals);
  if (sum_vals > 0) {
    return true;
  }
  else {
    return false;
  }
}

//функци управления процессом термопауз
void pause_control_function(bool permission);
void pause_control_function(bool permission){
  if (permission) {
  
    static bool timerStart = true; //Флаг для запуска таймера при достижении заданной температруры
    static bool flagCycl = false; //Флаг для запуска цикла
    digitalWrite(LEDSTART, HIGH);
    timesValue = vals[valsIndex]; //Передаём Время паузы цыкла
    tempValue = vals[valsIndex+1]; //Передаём Температура паузы цыкла 

    if (TE1 <= tempValue - 1) { 
      pid.setpoint = tempValue; //Передаём температуру в пид регулятор до момента TE - 1
      f_pid();
    }
    else {
      pid.setpoint = 0; //Передаём температуру в пид регулятор 0, для избежания перерегулирования
      f_pid();
    }

    if (TE1 >= tempValue - 1) {  // 
        
        if (timerStart) {
        
          totalCurrentSec = (timesValue * 60ul) + 1 ; // передаём в секундах  
          currentMillis = millis();
          
          timerStart = false;
          flagCycl = true;
        }

        if (flagCycl) {
          static uint32_t tmr4;
          //Это просто таймер которые отсчитыввает время и показывает его на экране
          if (millis() - tmr4 >= 1000) {
            tmr4 = millis();
            if (totalCurrentSec > 0) totalCurrentSec -= 1;
            timeCyclHours = (totalCurrentSec / 3600ul);        // часы
            timeCyclMins = (totalCurrentSec % 3600ul) / 60ul;  // минуты
            timeCyclSecs = (totalCurrentSec % 3600ul) % 60ul;  // секунды
          }
          
          if (totalCurrentSec <= 0 ) {
            timerStart = true;
            flagCycl = false;
              if (valsIndex < sizeof(vals) / sizeof(vals[0]) - 2) {
              valsIndex += 2;
              } else {
                // Достигнут конец массива vals, останавливаем цикл
                valsIndex = 0; // Сброс индекса или установка его в нужное значение
                StrStp = false; // Предположим, что это ваш флаг для остановки
                settingsMainMenu[2] = settingsStart[0];//Записываем в главное меню 'Stop'
                startingCycle = false; // Остановка цикла
                timeCyclHours = 0; // часы
                timeCyclMins = 0;  // минуты
                timeCyclSecs = 0;  // секунды
            }
          }
        }
      }
  }
  else {
    digitalWrite(LEDSTART, LOW);
    pid.setpoint = 0;
    f_pid();
    valsIndex = 0; // Сброс индекса или установка его в нужное значение
    StrStp = false; // Предположим, что это ваш флаг для остановки
    startingCycle = false; // Остановка цикла

    timeCyclHours = 0; // часы
    timeCyclMins = 0;  // минуты
    timeCyclSecs = 0;  // секунды
    
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
  lcd.setCursor(0, 0); lcd.print(" Brewery V 1.01");
  lcd.setCursor(0, 1); lcd.print("");
  
  f_TE1();

  delay(1000);

  sensTE1.requestTemp();     // запрос температуры 1 датчика
  sensTE2.requestTemp();    // запрос температуры 2 вентелятора
  
  delay(1000);

  pinMode(RELE_5, OUTPUT);
  pinMode(LEDSTART, OUTPUT);
  pinMode(FANACTIVATION, OUTPUT);

  pid.setDirection(NORMAL); // направление регулирования (NORMAL/REVERSE). ПО УМОЛЧАНИЮ СТОИТ NORMAL - нагрев
  pid.setLimits(0, 255);    // пределы (ставим для 8 битного ШИМ). ПО УМОЛЧАНИЮ СТОЯТ 0 И 255
  
  delay(1000);

  lcd.clear();
  lcd.setCursor(4, 0); lcd.print("TE1 ");
  lcd.setCursor(8, 0); lcd.print(TE1);
  lcd.setCursor(4, 1); lcd.print("TE2 ");
  lcd.setCursor(8, 1); lcd.print(TE2);

  delay(5000);

  printMainMenu();
}

void loop() {

  f_timer(); //Запуск глобального таймера
  f_TE1(); //функция измерения температуры 1
  LED_indication(); //функция световой индикации
  cooling_case(); //функция для охлаждения корпуса, управление кулером
  pause_control_function(startingCycle); //Пуск циакла

  static int timeSecs_tmp = 0;
  enc1.tick();
  
  switch (menu) {

    case Menu::MainMenu: //Главное Меню
      if (enc1.isTurn()) {
        int increment = 0;  // локальная переменная направления
        // получаем направление   
        if (enc1.isRight()) increment = 1;
        if (enc1.isLeft()) increment  = -1;
        arrowPos += increment;  // двигаем курсор  
        arrowPos = constrain(arrowPos, 0, SETTINGS_MENU - 1); // ограничиваем

        increment = 0;  // обнуляем инкремент

        printMainMenu(); //Выводим на экран соответсвующие меню
        Serial.println(arrowPos);
        
      }
      if (enc1.isClick()) {
        if (arrowPos == 0) {
          printSettingsValue();
          menu = Menu::SettingsValue;
        }
        if (arrowPos == 1) {
          lcd.clear();
          printMainWindow();
          menu = Menu::MainWindow;
        }
        if (arrowPos == 2) {
          //sum_setting();
          int settingsArray = sum_setting();
          StrStp = !StrStp;
				  if (StrStp && settingsArray == true){
            settingsMainMenu[2] = settingsStart[1] ; //Записываем в главное меню 'Start'
            valsIndex = 0;
            startingCycle = true;
          }
          else {
            settingsMainMenu[2] = settingsStart[0]; //Записываем в главное меню 'Stop'
            startingCycle = false;
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
        //Serial.println("settingPosition -> "); Serial.println(settingPosition);
        printSettingsValue();
        
        if (arrowPos == SETTINGS_AMOUNT-1) {
          settingPosition = !settingPosition;
          arrowPos = 0;
          printMainMenu(); 
          menu = Menu::MainMenu;
        }
      }
      break;

    case Menu::MainWindow:
      
      printMainWindow();
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
}
