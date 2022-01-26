#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <EEPROM.h>
#include "RTClib.h"


#define downButton                        2
#define upButton                          3
#define confirmButton                     4
#define cancelButton                      5
#define ledOut                            9

#define hasAlarmAddress                   0
#define hourAlarmAddress                  1
#define minuteAlarmAddress                2

int menuOption            = 1,
    subMenuOption         = 0,
    minutes               = 0,
    hours                 = 0,
    configTime            = 1,
    ledOutValue           = 0;

unsigned int timer        = 0;
unsigned int alarmCount   = 0;

bool downFlag           = false,
     upFlag             = false,
     confirmFlag        = false,
     cancelFlag         = false,
     playingAlarmClock  = false,
     hasAlarm           = false,
     isWakingUp         = false;


RTC_DS1307 rtc;
LiquidCrystal_I2C lcd(0x3F, 16,2);

void configRtcDateAndAlarmsNumbers() {
  rtc.adjust(DateTime(2022, 1, 23, 22, 0, 0));
  EEPROM.write(hasAlarmAddress, 0);
}


void setup() {
    rtc.begin();
    Serial.begin(9600);
    if (!rtc.begin()) {
      Serial.println("Inicializando RTC!");
      while (1);
    }  
//    configRtcDateAndAlarmsNumbers();
    Wire.begin();
    lcd.init();
    lcd.backlight();
    int hasAlarmValue = EEPROM.read(hasAlarmAddress);
    hasAlarm = hasAlarmValue == 1;
    for(int x = 2; x <= 5; x ++){
      pinMode(x, INPUT_PULLUP);
    }
    pinMode(ledOut, OUTPUT);
}

void loop() {
  DateTime now = rtc.now();
  checkAlarmClocks(now);
  listenButtonClicks();

  if(isWakingUp) {
    showWakeUpMessage();
    changeBrightnessLed();
  }else {
    switch(menuOption) {
      case 1:
        showDateAndHour(now);
        break;
      case 2:
        showMenu2();
        break;
      case 3: 
        showMenu3();
        break;
    }
  }
  
}


String formatTime(char valueChar) {
   String stringConcat = "0";
   int value = (int)valueChar;
   stringConcat += value;
   return value < 10 ? stringConcat : String(value);
}

int convertAsciiToInt(char asciiValue) {
  return (int)asciiValue;
}

void showDateAndHour(DateTime now) {
  lcd.setCursor(3, 0);
  lcd.print(formatTime(now.day()));
  lcd.print('/');
  lcd.print(formatTime(now.month()));
  lcd.print('/');
  lcd.print(now.year());
  
  lcd.setCursor(4, 1);
  lcd.print(formatTime(now.hour()));
  lcd.print(':');
  lcd.print(formatTime(now.minute()));
  lcd.print(':');
  lcd.print(formatTime(now.second()));
}

bool pressedButton(int port, bool *flag) {
  if(!digitalRead(port) && !*flag) {
     *flag = true;
  }
  if(digitalRead(port) && *flag) {
    *flag = false;
    return true;
  }
  return false;
}

void showMenu2() {
  if(subMenuOption == 0) {
    lcd.setCursor(6,0);
    lcd.print("MENU");
    lcd.setCursor(0,1);
    lcd.print("CONFIG");
    lcd.setCursor(8,1);
    lcd.print("VOLTAR");
    if(pressedButton(confirmButton, &confirmFlag)) {
      lcd.clear();
      subMenuOption = 1;
    }
  } else {
    configAlarm();
  }
}

String convertIntToString(int value) {
  return String(value);
}

String formatIntInHourString(int hourConfig, int minuteConfig) {
  String hourFormated   = hourConfig <= 9 
    ? "0" + convertIntToString(hourConfig) 
    : convertIntToString(hourConfig);
  String minuteFormated = minuteConfig <= 9 
    ? "0" + convertIntToString(minuteConfig) 
    :  convertIntToString(minuteConfig);
  String result = hourFormated + ":" + minuteFormated;
  return result;
}

void showMenu3() {
  if(hasAlarm) {
    int alarmHour   = EEPROM.read(hourAlarmAddress);
    int alarmMinute = EEPROM.read(minuteAlarmAddress);
    lcd.setCursor(0,0);
    lcd.print("ALARME CONFIGURADO:");
    lcd.setCursor(0,1);
    lcd.print(formatIntInHourString(alarmHour, alarmMinute));
  }else {
    lcd.setCursor(0,0);
    lcd.print("SEM ALARMES");
  }
}

void configAlarm() {
  blinkCursor();
  if(pressedButton(confirmButton, &confirmFlag)) {
    lcd.clear();
    configTime ++;
    if(configTime > 3) configTime = 3;
  }
  if(pressedButton(upButton, &upFlag) && subMenuOption >= 1) {
    lcd.clear();
    if(configTime == 1) incrementHour();
    else incrementMinute();
  }

  if(pressedButton(downButton, &downFlag) && subMenuOption >= 1) {
    lcd.clear();
    if(configTime == 1) decrementHour();
    else decrementMinute();
  }

  if(pressedButton(cancelButton, &cancelFlag)) {
    lcd.clear();
    subMenuOption = 0;
    hours = 0;
    minutes = 0; 
  }

  if(configTime == 3) {
    lcd.setCursor(0,0);
    lcd.print("Alarme configurado");
    delay(1000);
    lcd.clear();
    EEPROM.write(hasAlarmAddress, 1);
    EEPROM.write(hourAlarmAddress, hours);
    EEPROM.write(minuteAlarmAddress, minutes);
    
    hours = 0;
    minutes = 0;
    configTime = 1;
    subMenuOption = 0;
    hasAlarm = true;
  }
  
}

void blinkCursor() {
  switch(configTime) {
    case 1: 
      if((millis() - timer) >= 500) {
        showHourAndMinutes();
     }
     if((millis() - timer) < 500){
        lcd.setCursor(0,0);
        lcd.print("__");
        lcd.print(":");
        lcd.print(formatTime(char(minutes)));
     }
     if((millis() - timer) >= 1000) {
        timer = millis();
     }
     break;
    case 2: 
      if((millis() - timer) >= 500) {
        showHourAndMinutes();
      }
      if((millis() - timer) < 500){
          lcd.setCursor(0,0);
          lcd.print(formatTime(char(hours)));
          lcd.print(":");
          lcd.print("__");
      }
      if((millis() - timer) >= 1000) {
          timer = millis();
      }
      break;
  }
}

void showHourAndMinutes() {
  lcd.setCursor(0,0);
  lcd.print(formatTime(char(hours)));
  lcd.print(":");
  lcd.print(formatTime(char(minutes)));
}

void incrementHour() {
    hours ++;
    if(hours > 23) hours = 0;
}

void incrementMinute() {
    minutes ++;
    if(minutes > 59) {
      minutes = 0;
      incrementHour();
    }
}

void decrementHour() {
    hours --;
    if(hours < 0) hours = 23;
}

void decrementMinute() {
    minutes --;
    if(minutes < 0) {
      minutes = 59;
    }
}

void checkAlarmClocks(DateTime now) {
  int currentHour   = convertAsciiToInt(now.hour());
  int currentMinute = convertAsciiToInt(now.minute());
  
  if(hasAlarm) {
    int alarmHour   = EEPROM.read(hourAlarmAddress);
    int alarmMinute = EEPROM.read(minuteAlarmAddress);

    if(currentHour == alarmHour && currentMinute == alarmMinute && !isWakingUp) {
      lcd.clear();
      isWakingUp = true;
    }
  }
}

void changeBrightnessLed() {
  if((millis() - alarmCount) >= 236 && ledOutValue < 255) {
    ledOutValue += 1;
    alarmCount = millis();
    Serial.println(ledOutValue, DEC);
  }
  analogWrite(ledOut, ledOutValue);
}

void stopAwakening() {
  isWakingUp = false;
  digitalWrite(ledOut, LOW);
  ledOutValue = 0;
}

void showWakeUpMessage() {
  lcd.setCursor(0,0);
  lcd.print("Acorde !!!");
  lcd.setCursor(5,1);
  lcd.print("DESATIVAR");
}

void listenButtonClicks() {
  if(pressedButton(confirmButton, &confirmFlag)) {
    menuOption = 2;
    lcd.clear();
  }
  
  if(pressedButton(cancelButton, &cancelFlag) && subMenuOption == 0) {
    lcd.clear();
    menuOption --;
    if(menuOption < 1) menuOption = 1;
  }

  if(pressedButton(upButton, &upFlag) && subMenuOption == 0 && menuOption > 1) {
    lcd.clear();
    menuOption ++;
    if(menuOption > 3) menuOption = 3;
  }

  if(pressedButton(downButton, &downFlag) && subMenuOption == 0) {
    lcd.clear();
    menuOption --;
    if(menuOption < 1) menuOption = 1;
  }

  if(pressedButton(cancelButton, &cancelFlag) && isWakingUp) {
    lcd.clear();
    stopAwakening();
  }
  
}
