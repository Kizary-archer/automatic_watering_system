#include <EEPROM.h>
#include "TM1637.h"
#include <MsTimer2.h>
#include <iarduino_RTC.h>
////////////Settings///////////////
#define TimeSensorHours 1000 //Час в памяти
#define TimeSensorDays 1001 //День в памяти
#define TimeSensorMonth 1002 // Месяц в памяти
#define keeper 1003 //Сторож первого запуска
#define countlog 1004 // размер лога
#define Hour 1005 // Час последней записи 
#define Wetlavelmin 1006 // минимальный уровень влажности
#define WetsensorPower 8 //подача питания на датчик влажности
#define WetlavelEditPower 7 //питание патенциометра
#define Button 6 //кнопка режима настройки
#define pomp 5 //помпа
#define DispPower 4 //питание дисплея
#define WetlavelEdit 1 //потенциометр
#define Wetlavelnow 0 // датчик влажности
TM1637 tm1637(3, 2); //Создаём объект класса TM1637, в качестве параметров передаём номера пинов подключения
iarduino_RTC time(RTC_DS1307);
void EEPROMwrite();
void EEPROMclear();
void switchTimer();
void help();
void WetlavelEditor();
void timerDelay();
void analize();
void memoryFull();

void setup()
{
  pinMode(Wetlavelnow, INPUT);
  pinMode(WetsensorPower, OUTPUT);
  pinMode(WetlavelEditPower, OUTPUT);
  pinMode(Button, INPUT_PULLUP);
  pinMode(pomp, OUTPUT);
  pinMode(DispPower, OUTPUT);
  digitalWrite(DispPower, HIGH);
  Serial.begin(9600);
  MsTimer2::set(500, switchTimer); // задаем период прерывания по таймеру 500 мс
  MsTimer2::start();
  time.begin();
  time.period(10);
  time.gettime();
  tm1637.init();
  tm1637.set(BRIGHT_DARKEST);
  Serial.println("v1.9.5");
  Serial.println(time.gettime("d-m-Y, H:i:s, D"));
  Serial.println("enter h for help");
  byte addr = EEPROM.read(countlog);
  int val = map(EEPROM.read(addr - 1), 0 , 255 , 0, 1023 );
  tm1637.display(val);
  if (EEPROM.read(keeper) == 0)
  {
    EEPROM.write(TimeSensorHours, time.Hours);
    EEPROM.write(TimeSensorDays, time.day);
    EEPROM.write(TimeSensorMonth, time.month);
    EEPROM.write(keeper, 1);
    EEPROM.write(countlog, 0);
    EEPROM.write(Hour, time.Hours);
    EEPROMwrite();
  }
  if (EEPROM.read(Wetlavelmin) == 0) EEPROM.write(Wetlavelmin, 150);//минимальный уровень влажности при не установленном вручную
}

void(* resetFunc) (void) = 0;//перезагрузка

void loop()
{
  time.gettime();
  if (time.Hours != EEPROM.read(Hour)) {
    MsTimer2::stop();
    EEPROMwrite();
    if (map (analogRead(Wetlavelnow), 0, 1023, 0, 100) < EEPROM.read(Wetlavelmin))
      watering (); //полив
    MsTimer2::start();
  }
}
void EEPROMwrite()
{
  unsigned short addr = EEPROM.read(countlog)+1;
  digitalWrite(WetsensorPower, HIGH);
  timerDelay(5000);
  EEPROM.write(addr, map (analogRead(Wetlavelnow), 0, 1023, 0, 100));
  digitalWrite(WetsensorPower, LOW);
  unsigned short val = EEPROM.read(addr);
  EEPROM.write(countlog, addr);
  tm1637.display(val);
}
void EEPROMclear(unsigned short ind)
{
  Serial.println("****** clear start ******");
  for (unsigned short i = 0; i < ind; i++)
  {
    Serial.print("value = ");
    Serial.println(i);
    EEPROM.write(i, 0);
  }
  EEPROM.write(keeper, 0);
  resetFunc();
}
void WetlavelEditor ()
{
  digitalWrite(WetlavelEditPower, HIGH);
  digitalWrite(WetsensorPower, HIGH);
  digitalWrite(pomp, LOW);
  tm1637.point(true);
  while (digitalRead(Button) == LOW)
  {
    Serial.print("LevelEdit = ");
    Serial.println(analogRead(WetlavelEdit));
    tm1637.display(analogRead(WetlavelEdit));
    delay(10000);
  }
  byte sensVal = constrain(map (analogRead(WetlavelEdit), 0, 1023, 0, 254), 100, 254); //ограничение уровня влажности 100-254
  EEPROM.write(Wetlavelmin, sensVal );
  Serial.println(analogRead(WetlavelEdit));
  tm1637.point(false);
  digitalWrite(WetlavelEditPower, LOW);
  byte addr = EEPROM.read(countlog);
  int val = map(EEPROM.read(addr - 1), 0 , 255 , 0, 1024 );
  tm1637.display(val);
}
void watering ()
{
  digitalWrite(pomp, HIGH);
  timerDelay(4000);
  digitalWrite(pomp, LOW);
}
void analize ()
{
  Serial.println("***");
  timerDelay(2000);
  Serial.println(analogRead(Wetlavelnow));
}
void timerDelay(unsigned short t)
{
  unsigned long ts = millis();
  while (1) {
    unsigned long currentMillis = millis();
    // Serial.println(currentMillis/1000);
    if (currentMillis - ts > t)break;
  }
}
void switchTimer()
{
  if (Serial.available() > 0)
  {
    byte val = Serial.read();
    if (val == 'r') EEPROMread(EEPROM.read(countlog));
    else if (val == 'a') EEPROMread(1024);
    else if (val == 'c') EEPROMclear(255);
    else if (val == 'C') EEPROMclear(1024); //после запуска функции нужно установить мин. влажность!!!
    else if (val == 'R') resetFunc();
    else if (val == 'A') analize();
    else if (val == 'W')  digitalWrite(pomp , ! digitalRead(pomp));
    else if (val == 'h') help();
    else
    {
      Serial.println("this command does not exist");
      Serial.println("enter h for help");
    }
    Serial.clear(); // очистка буфера !!!
  }
  if (digitalRead(Button) == LOW) WetlavelEditor();
}
void help()
{
  Serial.println("****** HELP ******");
  Serial.println("r - reading data in the memory");
  Serial.println("c - clear the memory");
  Serial.println("C - clear all the memory");
  Serial.println("h - help");
  Serial.println("R - Restart");
  Serial.println("a - Read all");
  Serial.println("A - Wet analize");
  Serial.println("W - PompActivate");
}
