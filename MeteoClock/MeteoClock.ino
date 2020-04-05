
// ------------------------- НАСТРОЙКИ --------------------
#define RESET_CLOCK 0       // сброс часов на время загрузки прошивки (для модуля с несъёмной батарейкой). Не забудь поставить 0 и прошить ещё раз!
#define SENS_TIME 30000     // время обновления показаний сенсоров на экране, миллисекунд
#define LED_MODE 0    // тип RGB светодиода: 0 - главный катод, 1 - главный анод

// управление яркостью
#define BRIGHT_CONTROL 1      // 0/1 - запретить/разрешить управление яркостью (при отключении яркость всегда будет макс.)
#define BRIGHT_THRESHOLD 150  // величина сигнала, ниже которой яркость переключится на минимум (0-1023)
#define LED_BRIGHT_MAX 255    // макс яркость светодиода СО2 (0 - 255)
#define LED_BRIGHT_MIN 10     // мин яркость светодиода СО2 (0 - 255)
#define LCD_BRIGHT_MAX 255    // макс яркость подсветки дисплея (0 - 255)
#define LCD_BRIGHT_MIN 10     // мин яркость подсветки дисплея (0 - 255)

#define BLUE_YELLOW 0       // жёлтый цвет вместо синего (1 да, 0 нет) но из за особенностей подключения жёлтый не такой яркий
#define DISP_MODE 1         // в правом верхнем углу отображать: 0 - год, 1 - день недели, 2 - секунды
#define WEEK_LANG 0         // язык дня недели: 0 - английский, 1 - русский (транслит)
#define DEBUG 1             // вывод на дисплей лог инициализации датчиков при запуске. Для дисплея 1602 не работает! Но дублируется через порт!
#define PRESSURE 1          // 0 - график давления, 1 - график прогноза дождя (вместо давления). Не забудь поправить пределы гроафика
#define CO2_SENSOR 1        // включить или выключить поддержку/вывод с датчика СО2 (1 вкл, 0 выкл)
#define DISPLAY_TYPE 1      // тип дисплея: 1 - 2004 (большой), 0 - 1602 (маленький)
#define DISPLAY_ADDR 0x27   // адрес платы дисплея: 0x27 или 0x3f. Если дисплей не работает - смени адрес! На самом дисплее адрес не указан

// пределы отображения для графиков
#define TEMP_MIN 15
#define TEMP_MAX 35
#define HUM_MIN 0
#define HUM_MAX 100
#define PRESS_MIN -100
#define PRESS_MAX 100
#define CO2_MIN 300
#define CO2_MAX 2000

// пины
#define BACKLIGHT 10
#define PHOTO A3

#define MHZ_RX 2
#define MHZ_TX 3

#define LED_R 9
#define LED_G 6
#define LED_B 5
#define BTN_PIN 4

#define BL_PIN 10     // пин подсветки дисплея
#define PHOTO_PIN 0   // пин фоторезистора

// библиотеки
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#if (DISPLAY_TYPE == 1)
LiquidCrystal_I2C lcd(DISPLAY_ADDR, 20, 4);
#else
LiquidCrystal_I2C lcd(DISPLAY_ADDR, 16, 2);
#endif

#include "RTClib.h"
RTC_DS3231 rtc;
DateTime now;

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;


#if (CO2_SENSOR == 1)
#include <MHZ19_uart.h>
MHZ19_uart mhz19;
#endif

#include <GyverTimer.h>
GTimer_ms sensorsTimer(SENS_TIME); // обновления показаний сенсоров на экране
GTimer_ms drawSensorsTimer(SENS_TIME);
GTimer_ms clockTimer(500);                            // два раза в секунду пересчитываем время и мигаем точками
GTimer_ms hourPlotTimer((long)4 * 60 * 1000);         // 4 минуты
GTimer_ms dayPlotTimer((long)1.6 * 60 * 60 * 1000);   // 1.6 часа
GTimer_ms plotTimer(240000);
GTimer_ms predictTimer((long)10 * 60 * 1000);         // 10 минут
GTimer_ms brightTimer(2000);                          // 2 сек

#include "GyverButton.h"
GButton button(BTN_PIN, LOW_PULL, NORM_OPEN);


int8_t hrs, mins, secs, mnth, day_val;
int8_t dayOfWeek; // Day of week 0-6 starting with Sunday, e.g. Sunday = 0, Saturday = 6
uint16_t year_val;
byte mode = 0;
/*
  0 часы и данные
  1 график температуры за час
  2 график температуры за сутки
  3 график влажности за час
  4 график влажности за сутки
  5 график давления за час
  6 график давления за сутки
  7 график углекислого за час
  8 график углекислого за сутки
*/

// переменные для вывода
float dispTemp;
byte dispHum;
int dispPres;
int dispCO2;
int dispRain;

// массивы графиков
int tempHour[15], tempDay[15];
int humHour[15], humDay[15];
int pressHour[15], pressDay[15];
int co2Hour[15], co2Day[15];
int delta;
uint32_t pressure_array[6];
uint32_t sumX, sumY, sumX2, sumXY;
float a, b;
byte time_array[6];

// LED Settings
#if (LED_MODE == 0)
byte LED_ON = (LED_BRIGHT_MAX);
byte LED_OFF = (LED_BRIGHT_MIN);
#else
byte LED_ON = (255 - LED_BRIGHT_MAX);
byte LED_OFF = (255 - LED_BRIGHT_MIN);
#endif

void setLED(byte color) {
  // сначала всё выключаем
  if (!LED_MODE) {
    analogWrite(LED_R, 0);
    analogWrite(LED_G, 0);
    analogWrite(LED_B, 0);
  } else {
    analogWrite(LED_R, 255);
    analogWrite(LED_G, 255);
    analogWrite(LED_B, 255);
  }
  switch (color) {    // 0 выкл, 1 красный, 2 зелёный, 3 синий (или жёлтый)
    case 0:
      break;
    case 1: analogWrite(LED_R, LED_ON);
      break;
    case 2: analogWrite(LED_G, LED_ON);
      break;
    case 3:
      if (!BLUE_YELLOW) analogWrite(LED_B, LED_ON);
      else {
        analogWrite(LED_R, LED_ON - 50);    // чутка уменьшаем красный
        analogWrite(LED_G, LED_ON);
      }
      break;
  }
}

void setupLed(){
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  setLED(0);
}

void setupBmeSensor(){
  bme.begin(0x76, &Wire);  
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF   );
}

void readBmeSensors(){
  bme.takeForcedMeasurement();
  dispTemp = bme.readTemperature();
  dispHum = bme.readHumidity();
  dispPres = (float)bme.readPressure() * 0.00750062;
}

void setupCO2Sensor(){
  mhz19.begin(MHZ_TX, MHZ_RX);
  mhz19.setAutoCalibration(false);
}

void readCO2Sensor(){
   dispCO2 = mhz19.getPPM();
}

void redrawLed(){
  //if (dispCO2 >= 1200) {
  //  if (dotFlag) setLED(1);
  //  else setLED(0);
  //}

  if (dispCO2 < 800) setLED(2);
  else if (dispCO2 < 1200) setLED(3);
  else if (dispCO2 >= 1200) setLED(1);
}

void setupTime(){
  Serial.println("Time: setup start");
  if (RESET_CLOCK || rtc.lostPower()){
    Serial.println("Time: date");
    Serial.println(F(__DATE__));
    Serial.println("Time: time");
    Serial.println(F(__TIME__));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("Time: adjusted");
  } else {
    Serial.println("Time: no adjustment");
  }
}

void selfTest(){
  Serial.println("Setup: self-test start");

  lcd.setCursor(0, 0);
  lcd.print(F("Self test:"));
  boolean status = true;
  
  // test time
  setLED(3);
  if (rtc.begin()) {
    Serial.println("Time: test ok");
  } else {
    Serial.println("Time: test error");
    status = false;
  }

  // test bme
  if (bme.begin(0x76, &Wire)) {
    Serial.println("bme: test ok");
  } else {
    Serial.println("bme: test error");
    status = false;
  }

  // co2
  setLED(3);
#if (CO2_SENSOR == 1)
  mhz19.begin(MHZ_TX, MHZ_RX);
  mhz19.setAutoCalibration(false);
  mhz19.getStatus();    // первый запрос, в любом случае возвращает -1
  delay(500);
  if (mhz19.getStatus() == 0) {
    Serial.println("CO2: test ok");
  } else {
    Serial.println("CO2: test error");
    status = false;
  }
#endif

  lcd.setCursor(3, 1);
  if (status) {
    lcd.print(F("All good"));
    Serial.println(F("All good"));
    setLED(2);
  } else {
    lcd.print(F("Check wires!"));
    Serial.println(F("Check wires!"));
    setLED(1);
    
    while (1) {
      delay(2000);
    }
  }
  
  Serial.println("Setup: self-test end");
}

void setupScreen(){
  Serial.println("Screen: setup start");
  
  pinMode(BACKLIGHT, OUTPUT);
  analogWrite(BACKLIGHT, LCD_BRIGHT_MAX);
  
  lcd.init();
  lcd.backlight();
  lcd.clear();

  loadClock();

  Serial.println("Screen: setup end");
}


void readSensors() {
#if (CO2_SENSOR == 1)
  readCO2Sensor();
#endif

  readBmeSensors();
}

void checkBrightness() {
  if (analogRead(PHOTO) < BRIGHT_THRESHOLD) {   // если темно
    analogWrite(BACKLIGHT, LCD_BRIGHT_MIN);
#if (LED_MODE == 0)
    LED_ON = (LED_BRIGHT_MIN);
#else
    LED_ON = (255 - LED_BRIGHT_MIN);
#endif
  } else {                                      // если светло
    analogWrite(BACKLIGHT, LCD_BRIGHT_MAX);
#if (LED_MODE == 0)
    LED_ON = (LED_BRIGHT_MAX);
#else
    LED_ON = (255 - LED_BRIGHT_MAX);
#endif
  }
}

void modesTick() {
  button.tick();
  boolean changeFlag = false;
  if (button.isClick()) {
    mode++;

#if (CO2_SENSOR == 1)
    if (mode > 8) mode = 0;
#else
    if (mode > 6) mode = 0;
#endif
    changeFlag = true;
  }
  if (button.isHolded()) {
    mode = 0;
    changeFlag = true;
  }

  if (changeFlag) {
    if (mode == 0) {
      lcd.clear();
      loadClock();
      drawClock(hrs, mins, 0, 0, 1);
      if (DISPLAY_TYPE == 1) drawData();
      drawSensors();
    } else {
      lcd.clear();
      loadPlot();
      redrawPlot();
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("setup");

  setupScreen();
  setupLed();
  
#if (DEBUG == 1)
  selfTest();
#endif

  setupBmeSensor();
  setupCO2Sensor();
  setupTime();

  setLED(1);

  //wait before start. Warmup :)
  delay(2000);

  readSensors();
  readTime();

  if (DISPLAY_TYPE == 1) {
    loadClock();
    drawClock(hrs, mins, 0, 0, 1);
    drawData();
  }
  drawSensors();
}

void loop() {
  if (brightTimer.isReady()) checkBrightness(); // яркость - 2 сек
  if (sensorsTimer.isReady()) readSensors();    // читаем показания датчиков с периодом SENS_TIME
#if (DISPLAY_TYPE == 1)
  if (clockTimer.isReady()) clockTick();        // два раза в секунду пересчитываем время и мигаем точками
  plotSensorsTick();                            // тут внутри несколько таймеров для пересчёта графиков (за час, за день и прогноз)
  modesTick();                                  // тут ловим нажатия на кнопку и переключаем режимы
  if (mode == 0) {                                  // в режиме "главного экрана"
    if (drawSensorsTimer.isReady()) drawSensors();  // обновляем показания датчиков на дисплее с периодом SENS_TIME
  } else {                                          // в любом из графиков
    if (plotTimer.isReady()) redrawPlot();          // перерисовываем график
  }
#else
  if (drawSensorsTimer.isReady()) drawSensors();
#endif
  redrawLed();
}
