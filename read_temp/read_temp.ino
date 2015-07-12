/*
  Single_Temp.pde - Example using the MAX6675 Library.
  Created by Ryan McLaughlin <ryanjmclaughlin@gmail.com>

  This work is licensed under a Creative Commons Attribution-ShareAlike 3.0 Unported License.
  http://creativecommons.org/licenses/by-sa/3.0/
*/

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

SoftwareSerial debugPort(9, 10); // RX, TX
LiquidCrystal_I2C lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin, BACKLIGHT_PIN, POSITIVE);

#define SSID ""
#define PASSWORD ""

ESP esp(&Serial, &debugPort, 3);

REST rest(&esp);

boolean wifiConnected = false;

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

// Setup Serial output and LED Pin  
// MAX6675 Library already sets pin modes for MAX6675 chip!
void setup() {
  lcd.begin(10, 2);
  lcd.home();
  
  temperature = temp.read_temp();
  lcd.print("init");
  lcd.print(temperature);
    
  Serial.begin(19200);
  debugPort.begin(19200);
  
  esp.enable();
  delay(500);
  esp.reset();
  delay(500);
  while(!esp.ready());
  
  delay(1000);

  debugPort.println("ARDUINO: setup rest client");
  if(!rest.begin("yourapihere-com-r2pgihowjx7x.runscope.net")) {
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

void hasReachedTargetTemperature(float temperature) {
  if (temperature > 70.0) {
    tone(8, 31, 2000); 
  }
}

void loop() {
  char response[266];
  esp.process();
  
  if(wifiConnected) {
    rest.get("/");
    if(rest.getResponse(response, 266) == HTTP_STATUS_OK){
      debugPort.println("ARDUINO: GET successful");
      debugPort.println(response);
    }
      
    delay(1000);

    temperature = temp.read_temp();
    hasReachedTargetTemperature(temperature);
    lcd.clear();
    lcd.home();
    lcd.print(temperature);
    lcd.print("C");
    
  }
}
