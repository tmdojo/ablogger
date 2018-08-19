////////////////////////////////////////////////////////////
//#define ECHO_TO_SERIAL // Allows serial output if uncommented
////////////////////////////////////////////////////////////
#include <Wire.h>
#include "RTClib.h"
#include <RTCZero.h>
#include <SPI.h>
#include <SD.h>

#define cardSelect 5  // Set the pin used for uSD
#define RED 13 // Red LED on Pin #13
// #define GREEN 8 // Green LED on Pin #8
#define VBATPIN A7    // Battery Voltage on Pin A7
#define SPEAKER A0
const int buttonPinA = 11;    // the number of the pushbutton pin
const int buttonPinB = 12;    // the number of the pushbutton pin
const int silentPin = 10;    // pin for set silent

/////////////// Global Objects ////////////////////
RTC_PCF8523 rtc8523; // external RTC to load time in setup
RTCZero rtc;  // internal RTC to be used for interrupts
File logfile;   // Create file object
char filename[15]; // Varialbe for log file name on SD card
float measuredvbat;   // Variable for battery voltage

//////////////    Setup   ///////////////////
void setup() {
  #ifdef ECHO_TO_SERIAL
    while (! Serial); // Wait until Serial is ready
    delay(100); // delay to wait Serial is ready
    Serial.begin(115200);
    Serial.println("\r\nA/B logger");
  #endif

  rtc.begin();    // Start the RTC in 24hr mode
  rtc8523.begin();

  if (! rtc8523.initialized()) {
    #ifdef ECHO_TO_SERIAL
      Serial.println("RTC8523 is NOT running!");
    #endif
    // following line sets the RTC to the date & time this sketch was compiled
    rtc8523.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  rtc8523.adjust(DateTime(F(__DATE__), F(__TIME__)));

  DateTime now = rtc8523.now();
  // Set the time
  rtc.setHours(now.hour());
  rtc.setMinutes(now.minute());
  rtc.setSeconds(now.second());
  // Set the date
  rtc.setDay(now.day());
  rtc.setMonth(now.month());
  rtc.setYear(now.year()-2000); // now.year() returns 2018 but rtc.setYear() expects 18
  //rtc.setYear(18);

}

/////////////////////   Loop    //////////////////////
void loop() {
  Serial.print("Time on rtc8523: ");
  DateTime now = rtc8523.now();

  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  Serial.print("Time on SAMD: ");

  Serial.print(rtc.getDay());
  Serial.print("/");
  Serial.print(rtc.getMonth());
  Serial.print("/");
  Serial.print(rtc.getYear()+2000);
  Serial.print(" ");
  Serial.print(rtc.getHours());
  Serial.print(":");
  if(rtc.getMinutes() < 10)
    Serial.print('0');      // Trick to add leading zero for formatting
  Serial.print(rtc.getMinutes());
  Serial.print(":");
  if(rtc.getSeconds() < 10)
    Serial.print('0');      // Trick to add leading zero for formatting
  Serial.print(rtc.getSeconds());

  Serial.println();
  delay(3000);
}
