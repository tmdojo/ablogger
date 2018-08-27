/*
 Low Power A/B test logger

 based on Jonathan Davies's Low Power logger for Feather M0 Adalogger

 Uses internal RTC and interrupts to put M0 into deep sleep.
 Output logged to uSD at intervals set

 Author:  Shunya Sato
 Date:        09-Aug-2018
 Version:     1.0

*/

////////////////////////////////////////////////////////////
//#define ECHO_TO_SERIAL // Allows serial output if uncommented
////////////////////////////////////////////////////////////
#include <Wire.h>
#include "RTClib.h"
#include <RTCZero.h>
#include <SPI.h>
#include <SD.h>

#include "sheet_music.h"

#define DEBOUNCE 5  // button debouncer, how many ms to debounce, 5+ ms is usually plenty
#define cardSelect 5  // Set the pin used for uSD
#define RED 13 // Red LED on Pin #13
// #define GREEN 8 // Green LED on Pin #8
#define VBATPIN A7    // Battery Voltage on Pin A7
#define SPEAKER A0
const int buttonPinA = 11;    // the number of the pushbutton pin
const int buttonPinB = 12;    // the number of the pushbutton pin
const int silentPin = 10;    // pin for set silent

extern "C" char *sbrk(int i); //  Used by FreeRAm Function

//////////////// Key Settings ///////////////////
const int SampleIntSeconds = 500;   //Simple Delay used for testing, ms i.e. 1000 = 1 sec

/////////////// Global Objects ////////////////////
RTC_PCF8523 rtc8523; // external RTC to load time in setup
RTCZero rtc;  // internal RTC to be used for interrupts
File logfile;   // Create file object
char filename[15]; // Varialbe for log file name on SD card
float measuredvbat;   // Variable for battery voltage

int* songs[] = {theEnd, mario1Up, marioPowerUp, marioCoin, shaka};
int* tempos[] = {theEnd_tempo, mario1Up_tempo, marioPowerUp_tempo, marioCoin_tempo, shaka_tempo};
int lens[] = {theEnd_length, mario1Up_length, marioPowerUp_length, marioCoin_length, shaka_length};
int nsongs = sizeof(songs)/sizeof(int);
enum songsType {
  THE_END = 0,
  MARIO_1UP,
  MARIO_POWER_UP,
  MARIO_COIN,
  SHAKA
};
enum stateType {
  SLEEP = 0,
  WAKE_BY_PIN_A,
  WAKE_BY_PIN_B,
  WAKE_BY_RTC,
  RUNNING
};
volatile byte state = SLEEP;
int minute_ticks[] = {0, 30}; // array of minutes to set alarm
//int minute_ticks[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
//                      31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,
//                      51,52,53,54,55,56,57,58,59};
int nticks = sizeof(minute_ticks)/sizeof(int);
int iticks = -1; // initialize with -1 so that it would be used from zero
// float snooze = false; // TODO: Implement snooze functionality

//////////////    Setup   ///////////////////
void setup() {
  #ifdef ECHO_TO_SERIAL
    while (! Serial); // Wait until Serial is ready
    delay(100); // delay to wait Serial is ready
    Serial.begin(115200);
    Serial.println("\r\nA/B logger");
  #endif

  pinMode(buttonPinA, INPUT_PULLUP);
  pinMode(buttonPinB, INPUT_PULLUP);
  pinMode(silentPin, INPUT_PULLUP);

  pinMode(RED, OUTPUT);
  digitalWrite(RED, LOW);

  rtc.begin();    // Start the RTC in 24hr mode
  rtc8523.begin();

  //  if (! rtc8523.initialized()) {
  //    #ifdef ECHO_TO_SERIAL
  //      Serial.println("RTC8523 is NOT running!");
  //    #endif
  //    // following line sets the RTC to the date & time this sketch was compiled
  //    rtc8523.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //  }
  if (digitalRead(buttonPinA) == LOW) {
    rtc8523.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  DateTime now = rtc8523.now();
  // Set the time
  rtc.setHours(now.hour());
  rtc.setMinutes(now.minute());
  rtc.setSeconds(now.second());
  // Set the date
  rtc.setDay(now.day());
  rtc.setMonth(now.month());
  rtc.setYear(now.year()-2000);  // now.year() returns 2018 but rtc.setYear() expects 18

  strcpy(filename, "ANALOG.CSV");

  #ifdef ECHO_TO_SERIAL
    Serial.println("Logging ....");
  #endif
}

/////////////////////   Loop    //////////////////////
void loop() {
  #ifdef ECHO_TO_SERIAL
    Serial.println("start of loop function");
    Serial.print("State: ");
    Serial.println(state);
  #endif

  if (state == SLEEP) {
    // suporsed to run only once after reset
    #ifdef ECHO_TO_SERIAL
      Serial.println("SLEEP");
    #endif
    go_sleep();
  }
  else if (state == WAKE_BY_PIN_A) {
    // ButtonA is pressed
    #ifdef ECHO_TO_SERIAL
      Serial.println("WAKE_BY_PIN_A");
    #endif
    detach_all();
    log_btn(1);
    blink(RED, 8);
    go_sleep();
  }
  else if (state == WAKE_BY_PIN_B) {
    // this loop runs at every hour
    #ifdef ECHO_TO_SERIAL
      Serial.println("WAKE_BY_PIN_B");
    #endif
    detach_all();
    log_btn(2);
    blink(RED, 4);
    go_sleep();
  }
  else if (state == WAKE_BY_RTC) {
    // this loop runs at every hour
    #ifdef ECHO_TO_SERIAL
      Serial.println("WAKE_BY_RTC");
    #endif
    detach_all();
    if (digitalRead(silentPin) == HIGH){
      play_a_song(MARIO_COIN);
    }
    blink(RED, 2);
    go_sleep();
  }
}

///////////////   Functions   //////////////////

void log_btn(int btn) {
  #ifdef ECHO_TO_SERIAL
    SerialOutput(btn);           // Only logs to serial if ECHO_TO_SERIAL is uncommented at start of code
  #endif

  if (SdCardExist()){
    bool needHeader = !SD.exists(filename);
    logfile = SD.open(filename, FILE_WRITE);
    if( ! logfile ) {
      #ifdef ECHO_TO_SERIAL
        Serial.print("Couldnt create ");
        Serial.println(filename);
      #endif
      error(3);
      return;
    }
    else {
      #ifdef ECHO_TO_SERIAL
        Serial.print("Writing to ");
        Serial.println(filename);
      #endif
      if (needHeader) {
        writeHeader();
      }
      SdOutput(btn);                 // Output to uSD card
      logfile.flush();
      logfile.close();
      SD.end();
    }
  }
}

bool SdCardExist(){
  // see if the card is present and can be initialized:
  if (!SD.begin(cardSelect)) {
    #ifdef ECHO_TO_SERIAL
      Serial.println("Card init. failed! or Card not present");
    #endif
    error(2);     // Two red flashes means no card or card init failed.
    return false;
  } else {
    return true;
  }
}

// Write data header.
void writeHeader() {
  logfile.println("DD/MM/YYYY hh:mm:ss, Button, Battery Voltage");
}

// Debbugging output of time/date and battery voltage
void SerialOutput(int pressedButton) {

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
  Serial.print(",");
  Serial.print(pressedButton);   // Print battery voltage
  Serial.print(",");
  Serial.print(BatteryVoltage ());   // Print battery voltage
  Serial.print(",");
  Serial.println(freeram ());
}

// Print data and time followed by battery voltage to SD card
void SdOutput(int pressedButton) {

  //if (!file.sync() || file.getWriteError()) {
  //  error("write error");
  //  error(3);     // Three red flashes means write failed.
  //}

  // Formatting for file out put dd/mm/yyyy hh:mm:ss, [sensor output]
  uint8_t day = rtc.getDay();
  if (day < 10){
    logfile.print("0");
  }
  logfile.print(day);
  logfile.print("/");
  uint8_t month = rtc.getMonth();
  if (month < 10){
    logfile.print("0");
  }
  logfile.print(month);
  logfile.print("/");
  logfile.print(rtc.getYear()+2000);
  logfile.print(" ");
  uint8_t hour = rtc.getHours();
  if (hour < 10){
    logfile.print("0");
  }
  logfile.print(hour);
  logfile.print(":");
  uint8_t minute = rtc.getMinutes();
  if(minute < 10)
    logfile.print('0');      // Trick to add leading zero for formatting
  logfile.print(minute);
  logfile.print(":");
  uint8_t second = rtc.getSeconds();
  if(second < 10)
    logfile.print('0');      // Trick to add leading zero for formatting
  logfile.print(second);
  logfile.print(",");
  logfile.print(pressedButton);   // Print selected button
  logfile.print(",");
  logfile.println(BatteryVoltage ());   // Print battery voltage
}

void go_sleep() {
    setAlarm();

    #ifdef ECHO_TO_SERIAL
      //Serial.end();
      //USBDevice.detach(); // Safely detach the USB prior to sleeping
    #endif

    attachInterrupt(digitalPinToInterrupt(buttonPinA), pinA_isr, LOW);
    attachInterrupt(digitalPinToInterrupt(buttonPinB), pinB_isr, LOW);

    #ifndef ECHO_TO_SERIAL
      rtc.standbyMode();    // Sleep until next alarm match
      //standby();
      // Code re-starts here after sleep !
    #endif

    #ifdef ECHO_TO_SERIAL
      //USBDevice.attach();   // Re-attach the USB, audible sound on windows machines
      //delay(1000);  // Delay added to make serial more reliable
      //Serial.begin(115200);
      //while (! Serial); // Wait until Serial is ready
      //delay(100); // delay to wait Serial is ready
    #endif

}

void detach_all() {
    detachInterrupt(digitalPinToInterrupt(buttonPinA));
    detachInterrupt(digitalPinToInterrupt(buttonPinB));
    rtc.detachInterrupt();
    rtc.disableAlarm();
}

void pinA_isr() {
  state = WAKE_BY_PIN_A;
}
void pinB_isr() {
  state = WAKE_BY_PIN_B;
}
void rtc_isr() {
  state = WAKE_BY_RTC;
}

void setAlarm(){
  //setAlarmSec(10);
  //setAlarmMin(1);
  //setAlarmHour(1);
  setAlamAtMinute();

  #ifdef ECHO_TO_SERIAL
    Serial.print("Curret time: ");
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
    Serial.println(rtc.getSeconds());

    Serial.print("Wake up at: ");
    Serial.print(rtc.getAlarmDay());
    Serial.print("/");
    Serial.print(rtc.getAlarmMonth());
    Serial.print("/");
    Serial.print(rtc.getAlarmYear()+2000);
    Serial.print(" ");
    Serial.print(rtc.getAlarmHours());
    Serial.print(":");
    if(rtc.getAlarmMinutes() < 10)
      Serial.print('0');      // Trick to add leading zero for formatting
    Serial.print(rtc.getAlarmMinutes());
    Serial.print(":");
    if(rtc.getAlarmSeconds() < 10)
      Serial.print('0');      // Trick to add leading zero for formatting
    Serial.println(rtc.getAlarmSeconds());

    Serial.println("Good night!");
  #endif
}

void setAlarmSec(int seconds){
  ///////// Interval Timing and Sleep Code ////////////////
  int NextAlarmSec = rtc.getSeconds();
  NextAlarmSec = (NextAlarmSec + seconds) % 60;   // i.e. 65 becomes 5
  rtc.setAlarmSeconds(NextAlarmSec); // RTC time to wake, currently seconds only
  rtc.enableAlarm(rtc.MATCH_SS); // Match seconds only
  rtc.attachInterrupt(rtc_isr); // Attaches function to be called, currently blank
  delay(50); // Brief delay prior to sleeping not really sure its required
}
void setAlarmMin(int minutes){
  ///////// Interval Timing and Sleep Code ////////////////
  int NextAlarmMin = rtc.getMinutes();
  NextAlarmMin = (NextAlarmMin + minutes) % 60;   // i.e. 65 becomes 5
  rtc.setAlarmMinutes(NextAlarmMin); // RTC time to wake, currently seconds only
  rtc.enableAlarm(rtc.MATCH_MMSS); // Match minutes
  rtc.attachInterrupt(rtc_isr); // Attaches function to be called, currently blank
  delay(50); // Brief delay prior to sleeping not really sure its required
}
void setAlarmHour(int hours){
  ///////// Interval Timing and Sleep Code ////////////////
  int NextAlarmHour = rtc.getHours();
  NextAlarmHour = (NextAlarmHour + hours) % 24;   // i.e. 65 becomes 5
  rtc.setAlarmHours(NextAlarmHour); // RTC time to wake, currently seconds only
  rtc.enableAlarm(rtc.MATCH_HHMMSS); // Match seconds only
  rtc.attachInterrupt(rtc_isr); // Attaches function to be called, currently blank
  delay(50); // Brief delay prior to sleeping not really sure its required
}
void setAlamAtMinute() {
  ///////// Interval Timing and Sleep Code ////////////////
  int current_minute = rtc.getMinutes();
  for (int i=0; i<nticks; i++){
    if(minute_ticks[i] <= current_minute) {
      // current minute is after the tick
      if(i==(nticks-1)){
        // if it was the last tick, set alarm for the first tick
        rtc.setAlarmMinutes(minute_ticks[0]);
        break;
      }
      else {
        // otherwise check next tick
        continue;
      }
    }
    else {
      // current minute is before the tick thus set alarm for the tick
      rtc.setAlarmMinutes(minute_ticks[i]);
      break;
    }
  }
  rtc.enableAlarm(rtc.MATCH_MMSS); // Match minutes
  rtc.attachInterrupt(rtc_isr); // Attaches function to be called, currently blank
  delay(50); // Brief delay prior to sleeping not really sure its required
}

// blink out an error code
void error(uint8_t errno) {
  for (int j = 0; j<10; j++) {
    uint8_t i;
    for (i=0; i<errno; i++) {
      digitalWrite(RED, HIGH);
      delay(50);
      digitalWrite(RED, LOW);
      delay(50);
    }
    for (i=errno; i<10; i++) {
      delay(200);
    }
  }
}

// blink out an error code, Red on pin #13
void blink(uint8_t LED, uint8_t flashes) {
  uint8_t i;
  for (i=0; i<flashes; i++) {
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(200);
  }
}

// Measure battery voltage using divider on Feather M0 - Only works on Feathers !!
float BatteryVoltage () {
  measuredvbat = analogRead(VBATPIN);   //Measure the battery voltage at pin A7
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  return measuredvbat;
}

int freeram () {
  char stack_dummy = 0;
  return &stack_dummy - sbrk(0);
}

void play_a_song(int s){
  int* song = songs[s];
  int* tempo = tempos[s];
  int len = lens[s];

  int size = len / sizeof(int);
  for (int thisNote = 0; thisNote < size; thisNote++) {
    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    //TODO: make 1000 variable to change speed
    int noteDuration = 1000 / tempo[thisNote];
    tone(SPEAKER, song[thisNote], noteDuration);
    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.3;
    // stop the tone playing:
    delay(pauseBetweenNotes);
    noTone(SPEAKER);
  }
}
