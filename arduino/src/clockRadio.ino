/*
 Transistor 1 = GND / D12(via 100ohm resistor) / LED-P0
 Transistor 2 = GND / D13(via 100ohm resistor) / LED-P1
 Temp Sensor  = GND / A0 / +5v
 */

#include "LTC637D1P.h"
#include <dht11.h>
#include <Wire.h>
#include <RTClib.h>
#include <SoftwareSerial.h>

#define VERSION "v2.0.3"
//#define DIAG

const int G1       = 6; // cathode 1 for LED display
const int G2       = 7; // cathode 2 for LED display
const int CLOCK1   = 8; 
const int LATCH1   = 9;
const int DATA1    = 10;
const int CLOCK2   = 11;
const int LATCH2   = 12;
const int DATA2    = 5; // was in D13, moved to stopped the blinking LED
const int DHT11PIN = 2;
const int REFRESH_DELAY = 10;

// Digital Humidity and Temperature sensor
dht11 DHT11;

// Bluetooth serial adapter
SoftwareSerial btSerial(3, 4);

// LED display
View view(G1, G2, CLOCK1, CLOCK2, LATCH1, LATCH2, DATA1, DATA2);

// Real Time Clock
RTC_DS1307 RTC;

// which cathode to use for the LED display (false=1, true=2)
boolean phase = false;

short lastScreen = 0;
unsigned long startOfDayMillis = 0;
unsigned long lastSyncTime = 0;
unsigned long lastUpdate = 0;
uint16_t yr = 0;
uint8_t  mth = 0;
uint8_t  day = 0;
uint8_t  hr = 0;
uint8_t  min = 0;
uint8_t  sec = 0;
bool rtcMode = false;

void setup() {
  // start up the I2C comms 
  Wire.begin();
  
  // start the RTC
  RTC.begin();
  if(RTC.isrunning()) {
    rtcMode = true;
  }  

  Serial.begin(9600);
  Serial.print("SUPER CLOCK ");
  Serial.print(VERSION);
  Serial.println(" (Terminal)");
  Serial.println(rtcMode ? "RTC Mode enabled" : "RTC Mode disabled");

  btSerial.begin(9600);
  btSerial.print("SUPER CLOCK ");
  btSerial.print(VERSION);
  btSerial.print(" (Bluetooth)");
  btSerial.println(rtcMode ? "RTC Mode enabled" : "RTC Mode disabled");
}

#ifndef DIAG
void loop() {

  // update the character buffer
  timeTempHumidity();

  // commit the character buffer to the display
  view.write(phase);
  
  // handle any input coming from Bluetooth
  handleBluetoothInput();

  // switch the phase (effectively switches the cathode)
  phase = !phase;
  
  // allow the CPU to rest
  delay(REFRESH_DELAY);
}
#endif

/**
 * Test the display by cycling through the character set
 */
#ifdef DIAG
void diagnostic() {
  int seconds = (millis() / 1000);
  int x = seconds % 26;
  for(int i=0; i<4; i++) {
    view.setChar(i, x + 'A');
  }
}
#endif

void handleBluetoothInput() {
  if (btSerial.available()) {
    char command = (char)btSerial.read();

    if(command == '\n' || command == '\r')
      return;
      
    Serial.print("RX cmd=");
    Serial.println(command);

    if(command == 'f') {
      btSerial.read();
      btSerial.print("f=");
      btSerial.println(VERSION);
    } 
    else if(command == 'd') {
      btSerial.read();
      btSerial.print("d=");
      btSerial.print(day, DEC);
      btSerial.print('/');
      btSerial.print(mth, DEC);
      btSerial.print('/');
      btSerial.print(yr, DEC);   
      btSerial.print('T');
      btSerial.print(hr, DEC);
      btSerial.print(':');
      btSerial.print(min, DEC);
      btSerial.print(':');
      btSerial.println(sec, DEC);
    } 
    else if(command == 't') {
      btSerial.print("t=");
      btSerial.println(DHT11.temperature);
    } 
    else if(command == 'h') {
      btSerial.print("h=");
      btSerial.println(DHT11.humidity);
    }
    else if(command == 'u') {
      btSerial.print("u=");
      btSerial.println(millis());
    }
    else if(command == 'l') {
      btSerial.print("l=");
      btSerial.println(millis() - lastSyncTime);
    }
    else if(command == 'r') {
      btSerial.print("r=");
      btSerial.println(rtcMode ? 1 : 0);
    }
    else if(command == 's') {
      day = btSerial.parseInt();
      btSerial.read();
      mth = btSerial.parseInt();
      btSerial.read();
      yr = btSerial.parseInt();
      btSerial.read();
      hr = btSerial.parseInt();
      btSerial.read();
      min = btSerial.parseInt();
      btSerial.read();
      sec = btSerial.parseInt();
      btSerial.read();

      uint32_t oldTime = RTC.now().unixtime();
      RTC.adjust(DateTime(yr, mth, day, hr, min, sec));
      uint32_t newTime = RTC.now().unixtime();
      uint32_t timeDiff = newTime - oldTime;

      Serial.print("  New Time: ");
      Serial.print(day);
      Serial.print("/");
      Serial.print(mth);
      Serial.print("/");
      Serial.print(yr);
      Serial.print(" ");
      Serial.print(hr);
      Serial.print(":");
      Serial.print(min);
      Serial.print(":");
      Serial.println(sec);
      Serial.print("Adjustment: ");
      Serial.print(timeDiff / 1000.0);
      Serial.println("s");

      unsigned long secondOfDay = (hr * 60l * 60l) + (min * 60l);
      startOfDayMillis = millis() - (secondOfDay * 1000);
      lastSyncTime = millis();
      
      btSerial.print("s=");
      btSerial.println(timeDiff / 1000.0);
    }
  }
}

/**
 * The display is on a 10 seconds cycle:
 * - time for 6 seconds
 * - temperature for 2 seconds
 * - humidity for 2 seconds
 */
void timeTempHumidity() {

  // calculate where we are in the cycle (0-9 seconds)
  byte time = ((millis() % 10000) / 1000);

  if(time < 6) {
    writeTimeValue();
    lastScreen = 0;
  } 
  else if(time < 8) {
    // only perform the temp/humidity read when switching screen    
    if(lastScreen != 1) {
      readTempAndHumidity();
    }
    writeTemp();
    lastScreen = 1;
  } 
  else if(time < 10) {
    writeHumidity();
    lastScreen = 2;
  }
}

/**
 * Interrogate the DHT11 and request it to read values. 
 * Note: This is a slow operation and can make the display flicker. It is best 
 *       to do the read while the display is switching
 */
void readTempAndHumidity() {
  DHT11.read(DHT11PIN);
}

/**
 * Write the temperature to the character buffer
 * Format is " t23"
 */
void writeTemp() {
  view.setCharAndDigit('T', (int)DHT11.temperature);
  view.setDotState(DOT_AM, false);
  view.setDotState(DOT_PM, false);
  view.setDotState(DOT_MID, false);
}

/**
 * Write the humidity value to the character buffer
 * Format is " h54"
 */
void writeHumidity() {
  view.setCharAndDigit('H', (int)DHT11.humidity);
  view.setDotState(DOT_AM, false);
  view.setDotState(DOT_PM, false);
  view.setDotState(DOT_MID, false);
}

/**
 * Write the time to the character buffer
 * Format is "HH:MM" (24hr time)
 */
void writeTimeValue() {    
  if(millis() - lastUpdate > 1000) {
    if(rtcMode) {
      updateTimeValueFromRTC();
    } 
    else {
      updateTimeValueFromMillis();
    }
    lastUpdate = millis();
  }

  view.setDigits(hr, min);
  view.setDotState(DOT_AM, false);
  view.setDotState(DOT_PM, false);
  view.setDotState(DOT_MID, !((millis() / 500) % 2));
}

/**
 * Read the time from the RTC
 * Note: this is a slow operation and cause the display to flicker
 */
void updateTimeValueFromRTC() {
  DateTime now = RTC.now();

  if(now.hour() < 24 && now.minute() < 60 && now.second() < 60) {    
    hr = now.hour();
    min = now.minute();  
    sec = now.second();
    
    if(now.year() > 2012 && now.year() < 2100 && now.month()<= 12 && now.day() <= 31) {
      yr = now.year();
      mth = now.month();
      day = now.day();
    }
  }
}

/** 
 * Work out what the time is when RTC is not available. 
 * Time needs to be initially set via Bluetooth.
 */
void updateTimeValueFromMillis() {
  unsigned long now = millis();
  unsigned long secondOfDay = (now - startOfDayMillis) / 1000;
  byte secondOfMinute = secondOfDay % 60;
  byte minuteOfHour = (secondOfDay / 60) % 60;
  byte hourOfDay = (secondOfDay / 60 / 60) % 24;  

  if(secondOfDay >= 86400) {
    startOfDayMillis += 86400000;
  }  
}






