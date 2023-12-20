#include <Arduino.h>

#define LINES 2
#define SETTINGS_AMOUNT 13
#define SETTINGS_SETTING 6

bool controlState = false;

#define CLK 2
#define DT 3
#define SW 4

enum class Menu { MainMenu, MainWindow, SettingsValue, StartStopSettings };

#include <GyverEncoder.h>
Encoder enc1(CLK, DT, SW);

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

int vals[SETTINGS_SETTING];
int8_t arrowPos = 0;
int8_t entVal = 0;
int8_t screenPos = 0;
int8_t dataEntry = 0;

bool checkTE1 = false;

Menu menu;

String settingsValue[] = {
  "Time-1",
  "Temp-1",
  "Time-2",
  "Temp-2",
  "Time-3",
  "Temp-3",
  "Exit",
};

String settingsMainMenu[] = {
  "Setting",
  "Window",
  "Stop",
};

String settingsStart[] = {
  "Stop",
  "Start",
};

#include "GyverTimer.h" 

GTimer TimerBRW_1(MS);

uint32_t period = 1000;

bool fBrwStart = false, fTimerStart = true, timerStart = false;
int valsIndex = 0, k = 0, ai = 0, ti = 0, fulTim = 0, summ_arr = 0;
uint32_t timer_brw1, timer_brw2, timer_brw3;

uint32_t totalMills;
int timeHours;
int timeMins = 0;
int timeSecs;

uint32_t totalCyclMills = 0;
uint32_t totalCyclsec = 0;

int timeCyclHours;
int timeCyclMins = 0;
int timeCyclSecs = 0;

#include <GyverPID.h>

GyverPID pid(7.72, 0.41, 0);
int PIDperiod = 500;

#define RELE_5 5

#include <microDS18B20.h>
MicroDS18B20<6> sensTE;

int temp1, TE1;
int tempValue;

int readTemperature() {
  sensTE.readTemp();
  int temp = sensTE.getTempInt();
  sensTE.requestTemp();
  return temp;
}

void printMainWindow() {
  lcd.setCursor(0, 0); lcd.print("t:"); lcd.print(TE1); lcd.print("C");
  lcd.setCursor(8, 0); lcd.print("T:"); lcd.print(timeCyclMins); lcd.print(":"); lcd.print(timeCyclSecs);
  lcd.setCursor(0, 1); lcd.print("t:"); lcd.print(tempValue); lcd.print("C");
  lcd.setCursor(8, 1); lcd.print("T:"); lcd.print(timeMins); lcd.print(" min");
}

void printSettingsValue() {
  lcd.clear();
  screenPos = arrowPos / LINES;

  for (byte i = 0; i < LINES; i++) {
    lcd.setCursor(0, i);

    if (arrowPos == LINES * screenPos + i) lcd.write(126);
    else lcd.write(32);

    if (LINES * screenPos + i == SETTINGS_AMOUNT) break;

    lcd.print(settingsValue[LINES * screenPos + i]);
    if (arrowPos < SETTINGS_SETTING) {
      lcd.print(": ");
      lcd.print(vals[LINES * screenPos + i]);
    }
  }
}

void printMainMenu() {
  lcd.clear();
  screenPos = arrowPos / LINES;

  for (byte i = 0; i < LINES; i++) {
    lcd.setCursor(0, i);

    if (arrowPos == LINES * screenPos + i) lcd.write(126);
    else lcd.write(32);

    if (LINES * screenPos + i == SETTINGS_MENU) break;

    lcd.print(settingsMainMenu[LINES * screenPos + i]);
  }
}

void updateTimer() {
  totalMills = millis();
  timeHours = (totalMills / 3600000ul) % 24ul;
  timeMins = (totalMills / 60000ul) % 60ul;
  timeSecs = (totalMills / 1000ul) % 60ul;
}

void updateCycleTimer() {
  totalCyclMills = millis() - totalMills;
  totalCyclsec = totalCyclMills / 1000ul;
  timeCyclHours = (totalCyclsec / 3600ul);
  timeCyclMins = (totalCyclsec % 3600ul) / 60ul;
  timeCyclSecs = (totalCyclsec % 3600ul) % 60ul;
}

void updatePID() {
  pid.input = TE1;
  pid.getResult();
  analogWrite(RELE_5, pid.output);
}

void setup() {
  Serial.begin(9600);

  enc1.setType(TYPE2);

  lcd.init();
  lcd.backlight();

  menu = Menu::MainMenu;

  Serial.println("Start");

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Brewery V 0.4");
  lcd.setCursor(0, 1); lcd.print("Turn the handle");

  TimerBRW_1.setInterval(100000);

  sensTE.requestTemp();
  delay(250);

  pinMode(RELE_5, OUTPUT);

  pid.setDirection(NORMAL);
  pid.setLimits(0, 255);
  delay(250);
}

void loop() {
  TE1 = readTemperature();

  enc1.tick();

  switch (menu) {
    case Menu::MainMenu:
      if (enc1.isTurn()) {
        int increment = 0;
        if (enc1.isRight()) increment = 1;
        if (enc1.isLeft()) increment = -1;
        arrowPos += increment;
        arrowPos = constrain(arrowPos, 0, SETTINGS_MENU - 1);

        increment = 0;

        printMainMenu();
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
          menu = Menu::StartStopSettings;
          lcd.clear();
          printMainMenu();
        }
      }
      break;

    case Menu::SettingsValue:
      if (enc1.isTurn()) {
        int increment = 0;
        if (enc1.isRight()) increment = 1;
        if (enc1.isLeft()) increment = -1;
        arrowPos += increment;
        arrowPos = constrain(arrowPos, 0, SETTINGS_AMOUNT - 1);

        increment = 0;

        if (arrowPos < SETTINGS_SETTING) {
          if (enc1.isRightH()) increment = 1;
          if (enc1.isLeftH()) increment = -1;

          vals[arrowPos] += increment;
          if (arrowPos % 2 == 0) {
            dataEntry = constrain(vals[arrowPos], 0, 60);
            vals[arrowPos] = dataEntry;
          }
          else {
            dataEntry = constrain(vals[arrowPos], 0, 100);
            vals[arrowPos] = dataEntry;
          }
        }
        printSettingsValue();
      }
      if (enc1.isClick() && arrowPos == SETTINGS_AMOUNT - 1) {
        arrowPos = 0;
        lcd.clear();
        printMainMenu();
        Serial.println("SettingsValue");
        menu = Menu::MainMenu;
      }
      break;

    case Menu::MainWindow:
      printMainWindow();
      if (enc1.isClick()) {
        lcd.clear();
        Serial.println("MainWindow");
        printMainMenu();
        menu = Menu::MainMenu;
      }
      break;

    case Menu::StartStopSettings:
      if (!fBrwStart) {
        fBrwStart = true;
        settingsMainMenu[2] = "Start";
        Serial.print("fBrwStart ");
        Serial.println(fBrwStart);
      }
      else {
        fBrwStart = false;
        settingsMainMenu[2] = "Stop";
        Serial.print("fBrwStart ");
        Serial.println(fBrwStart);
      }
      lcd.clear();
      printMainMenu();
      menu = Menu::MainMenu;
      break;
  }

  if (fBrwStart) {
    timeMins = vals[valsIndex];
    tempValue = vals[valsIndex + 1];

    if (TE1 >= tempValue - 1) {
      checkTE1 = true;
      if (fTimerStart) {
        updateTimer();
        updateCycleTimer();
        TimerBRW_1.setTimeout(vals[valsIndex] * 1000ul * 60ul);
        Serial.print("Timer start ");
        Serial.println(fTimerStart);
        Serial.print("Cycle time ");
        Serial.print(vals[valsIndex]);
        Serial.println(" min");
        Serial.print("Temperature ");
        Serial.print(vals[valsIndex + 1]);
        Serial.print(" C");
        Serial.print("  TE1= ");
        Serial.println(TE1);
        Serial.print("valsIndex= ");
        Serial.println(valsIndex);
        fTimerStart = false;
      }
      if (TimerBRW_1.isReady()) {
        Serial.print("End of cycle ");
        Serial.print(vals[valsIndex]);
        Serial.println(" min");
        Serial.print("               ");
        Serial.print(vals[valsIndex + 1]);
        Serial.println(" C");
        fTimerStart = true;
        valsIndex += 2;
      }
      if (valsIndex >= SETTINGS_SETTING) {
        valsIndex = 0;
        fBrwStart = false;
        fTimerStart = true;
        settingsMainMenu[2] = "Stop";
      }
    }
    else {
      checkTE1 = false;
    }

    if (TE1 <= tempValue) {
      pid.setpoint = tempValue;
      updatePID();
    }
    else {
      pid.setpoint = 0;
    }
  }
}
