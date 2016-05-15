
#include <avr/pgmspace.h>
#include <SoftwareSerial.h>
#include <MAX6675.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <espduino.h>
#include <rest.h>


int CS = 5;             // CS pin on MAX6675
int SO = 6;              // SO pin of MAX6675
int SCK1 = 4;             // SCK pin of MAX6675
int units = 1;            // Units to readout temp (0 = raw, 1 = ˚C, 2 = ˚F)
float temperature = 0.0;  // Temperature output variable
int targetTemperature = 0;

// Initialize the MAX6675 Library for our chip
MAX6675 temp(CS,SO,SCK1,units);

#define I2C_ADDR      0x27 // I2C address of PCF8574A
#define BACKLIGHT_PIN 3
#define En_pin        2
#define Rw_pin        1
#define Rs_pin        0
#define D4_pin        4
#define D5_pin        5
#define D6_pin        6
#define D7_pin        7


#define NOTE_C4 262
#define SPEAKER_PORT 8 
#define PICO_INPUT A0

#define BUTTON_PIN 2

SoftwareSerial debugPort(9, 10); // RX, TX
LiquidCrystal_I2C lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin, BACKLIGHT_PIN, POSITIVE);

#define SSID "example.com"
#define PASSWORD "PWD"

#define DOMAIN "example.com"
#define PATH "/mash_measurements"

#define TEMPERATURE_STR 0
const char preTmp[] PROGMEM = "mashing%5Btemperature%5D=";
const char* const stringTable[] PROGMEM = {preTmp};

ESP esp(&Serial, &debugPort, 3);

REST rest(&esp);

boolean wifiConnected = false;
boolean loggingDisabled = false;

void wifiCb(void* response)
{
  uint32_t status;
  RESPONSE res(response);

  if(res.getArgc() == 1) {
    res.popArgs((uint8_t*)&status, 4);
    if(status == STATION_GOT_IP) {
      debugPort.println("WIFI CONNECTED");
     
      wifiConnected = true;
    } else {
      wifiConnected = false;
    }
    
  }
}


void setup() {
  lcd.begin(10, 2);
  lcd.home();
  
  temperature = temp.read_temp();
  lcd.print("init: C: ");
  lcd.print(readTargetTemp());
    
  Serial.begin(19200);
  debugPort.begin(19200);
  debugPort.println("start");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), checkToggleLogging, RISING);
  
  esp.enable();
  delay(500);
  esp.reset();
  delay(500);
  while(!esp.ready());
  
  delay(1000); 
  
  debugPort.println("ARDUINO: setup rest client");
  if(!rest.begin(DOMAIN)) {
    debugPort.println("ARDUINO: failed to setup rest client");
    while(1);
  }
 
  delay(1000);

  /*setup wifi*/
  debugPort.println("ARDUINO: setup wifi");
  esp.wifiCb.attach(&wifiCb);
  esp.wifiConnect(SSID, PASSWORD);
  debugPort.println("ARDUINO: system started");
}

void checkToggleLogging() {
  delay(200);
  if (digitalRead(BUTTON_PIN)){
    loggingDisabled = !loggingDisabled;
  }
}

char* getProgMemStr(int strPosition) {
  return ((char*)pgm_read_word(&(stringTable[strPosition])));
}

void hasReachedTargetTemperature(float temperature, int targetTemperature) {
  if (temperature > targetTemperature) {
    tone(SPEAKER_PORT, NOTE_C4, 500); 
    delay(500);
  }
}

void copyAndCat(char* input, int strPosition, float measurement) {
  char tmp[20];
  char tempStr[50];
  dtostrf(measurement, 4, 3, tmp);
  strcpy_P(tempStr, getProgMemStr(strPosition));
  strcat(tempStr, tmp);
  strcat(tempStr, "\&");
  strcat(input, tempStr);
}

void createTemperatureStr(char* input, float temperature) {
  strcpy(input, "");
  copyAndCat(input, TEMPERATURE_STR, temperature);
}

int readTargetTemp() {
  return analogRead(PICO_INPUT) / 10;
}

void printTemperature(float temperature, float targetTemperature) {
  lcd.clear();
  lcd.home();
  lcd.print(temperature);
  lcd.print("C->");
  lcd.print(targetTemperature);
  lcd.println("C");

  lcd.setCursor(0, 1);
  if(loggingDisabled) {
    lcd.print("w: off");
  } else {
    lcd.print("w: on");
  }

  if(wifiConnected) {
    lcd.print("O");
  } else {
    lcd.print("-");
  }
};

char tmp[150];
int updateCount = 100;
void loop() {
  char response[266];
  esp.process();

  temperature = temp.read_temp();
  targetTemperature = readTargetTemp();
  hasReachedTargetTemperature(temperature, targetTemperature);

  updateCount--;
  
  if(wifiConnected && !loggingDisabled && updateCount == 0) {
    createTemperatureStr(tmp, temperature);
    rest.setContentType("application/x-www-form-urlencoded");
    rest.post(PATH, tmp);

    if(rest.getResponse(response, 266) == HTTP_STATUS_OK){
      debugPort.println("ARDUINO: GET successful");
      debugPort.println(response);
    }

   updateCount = 100;
   delay(200);
  }

  printTemperature(temperature, targetTemperature);

  delay(200);
}
