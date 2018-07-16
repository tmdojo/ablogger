/*
 Low Power A/B test logger

 based on Jonathan Davies's Low Power logger for Feather M0 Adalogger

 Uses internal RTC and interrupts to put M0 into deep sleep.
 Output logged to uSD at intervals set

 Author:  Shunya Sato
 Date:        15/07/18
 Version:     0.1

*/

////////////////////////////////////////////////////////////
#define ECHO_TO_SERIAL // Allows serial output if uncommented
////////////////////////////////////////////////////////////

#include <SPI.h>
#include <SD.h>
#include <RTCZero.h>

#include "pitches.h"

#define DEBOUNCE 5  // button debouncer, how many ms to debounce, 5+ ms is usually plenty
#define cardSelect 4  // Set the pin used for uSD
#define RED 13 // Red LED on Pin #13
#define GREEN 8 // Green LED on Pin #8
#define VBATPIN A7    // Battery Voltage on Pin A7
#define SPEAKER A0
const int buttonPinA = 11;    // the number of the pushbutton pin
const int buttonPinB = 12;    // the number of the pushbutton pin
const int silentPin = 10;    // pin for set silent


extern "C" char *sbrk(int i); //  Used by FreeRAm Function

//////////////// Key Settings ///////////////////

#define SampleIntSec 15 // RTC - Sample interval in seconds
#define SampleIntMin 1 // RTC - Sample interval in minutes
#define SampleIntHour 1 // RTC - Sample interval in hour

const int SampleIntSeconds = 500;   //Simple Delay used for testing, ms i.e. 1000 = 1 sec

const bool setTime = false;
/* Change these values to set the current initial time */
const byte hours = 14;
const byte minutes = 00;
const byte seconds = 0;
/* Change these values to set the current initial date */
const byte day = 15;
const byte month = 7;
const byte year = 18;

/////////////// Global Objects ////////////////////
RTCZero rtc;    // Create RTC object
File logfile;   // Create file object
char filename[15]; // Varialbe for log file name on SD card
float measuredvbat;   // Variable for battery voltage
int NextAlarmSec; // Variable to hold next alarm time in seconds
int NextAlarmMin; // Variable to hold next alarm time in minutes
int NextAlarmHour; // Variable to hold next alarm time in hours
int pressedButton;

// notes in the melody:
int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};


//////////////    Setup   ///////////////////
void setup() {

  rtc.begin();    // Start the RTC in 24hr mode
  if (setTime){
    rtc.setTime(hours, minutes, seconds);   // Set the time
    rtc.setDate(day, month, year);    // Set the date    
  }
  NextAlarmMin = rtc.getMinutes(); // initialize starting minute
  NextAlarmHour = rtc.getHours();  // initialize starting hour

  #ifdef ECHO_TO_SERIAL
    //while (! Serial); // Wait until Serial is ready
    delay(100); // delay to wait Serial is ready
    Serial.begin(115200);
    Serial.println("\r\nFeather M0 Analog logger");
  #endif

  // see if the card is present and can be initialized:
  if (!SD.begin(cardSelect)) {
    Serial.println("Card init. failed! or Card not present");
    error(2);     // Two red flashes means no card or card init failed.
  }
  
  strcpy(filename, "ANALOG00.CSV");
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i/10;
    filename[7] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }

  logfile = SD.open(filename, FILE_WRITE);
  if( ! logfile ) {
    #ifdef ECHO_TO_SERIAL
      Serial.print("Couldnt create ");
      Serial.println(filename);
    #endif
    error(3);
  }
  #ifdef ECHO_TO_SERIAL
    Serial.print("Writing to ");
    Serial.println(filename);
  #endif
  writeHeader();
  logfile.close();

  pinMode(buttonPinA, INPUT_PULLUP);
  pinMode(buttonPinB, INPUT_PULLUP);
  pinMode(silentPin, INPUT_PULLUP);

  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);

  #ifdef ECHO_TO_SERIAL
    Serial.println("Logging ....");
  #endif
}

/////////////////////   Loop    //////////////////////
void loop() {
  // this loop runs at every hour
  #ifdef ECHO_TO_SERIAL
    Serial.println("start of loop function");
  #endif

  pressedButton = 0;
  unsigned long startMillis = millis();
  //unsigned long expireMillis = millis() + (30 * 1000); // TEST: wait 30 sec for A/B input
  unsigned long expireMillis = millis() + (15 * 60 * 1000); // wait 15 minutes for A/B input
  if (digitalRead(silentPin) == HIGH){
    playTone();
  }
  blink(RED,2);             // Quick blink to show we have a pulse

  while(pressedButton==0){
//    #ifdef ECHO_TO_SERIAL
//      Serial.print(".");
//    #endif
    if ( (long)(millis()-expireMillis) >= 0 ){
      // break if 15 min has passed
      #ifdef ECHO_TO_SERIAL
        Serial.println("\r\n time out. Exit while");
      #endif
      break;
    }
    if ( (millis() - startMillis) > (10 * 1000) ) {
      #ifdef ECHO_TO_SERIAL
        Serial.println("\r\n 10 sec passed");
      #endif
      // blink every 10 seconds
      blink(RED,2);             // Quick blink to show we have a pulse
      startMillis = millis();
    }

    // check A/B buttons
    if (!digitalRead(buttonPinA)){
      delay(DEBOUNCE);
      if(!digitalRead(buttonPinA)){
        pressedButton = 1;
      }
    }
    else if (!digitalRead(buttonPinB)){
      delay(DEBOUNCE);
      if(!digitalRead(buttonPinB)){
        pressedButton = 2;
      }
    }
  }


  #ifdef ECHO_TO_SERIAL
    SerialOutput();           // Only logs to serial if ECHO_TO_SERIAL is uncommented at start of code
  #endif

  logfile = SD.open(filename, FILE_WRITE);
  if( ! logfile ) {
    #ifdef ECHO_TO_SERIAL
      Serial.print("Couldnt create ");
      Serial.println(filename);
    #endif
    error(3);
  }
  SdOutput();                 // Output to uSD card
  //logfile.flush();
  logfile.close();
  blink(GREEN,2); 
  
  //setAlarmSec();
  //setAlarmMin();
  setAlarmHour();
  
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

  #ifdef ECHO_TO_SERIAL
    Serial.end();
    USBDevice.detach(); // Safely detach the USB prior to sleeping
  #endif
  
  rtc.standbyMode();    // Sleep until next alarm match

  #ifdef ECHO_TO_SERIAL
    USBDevice.attach();   // Re-attach the USB, audible sound on windows machines
    Serial.begin(115200);
    //while (! Serial); // Wait until Serial is ready
    delay(100); // delay to wait Serial is ready
  #endif

  
  // Code re-starts here after sleep !
}

///////////////   Functions   //////////////////

void setAlarmSec(){
  ///////// Interval Timing and Sleep Code ////////////////
  delay(SampleIntSeconds);   // Simple delay for testing only interval set by const in header

  NextAlarmSec = (NextAlarmSec + SampleIntSec) % 60;   // i.e. 65 becomes 5
  rtc.setAlarmSeconds(NextAlarmSec); // RTC time to wake, currently seconds only
  rtc.enableAlarm(rtc.MATCH_SS); // Match seconds only
  rtc.attachInterrupt(alarmMatch); // Attaches function to be called, currently blank
  delay(50); // Brief delay prior to sleeping not really sure its required
}
void setAlarmMin(){
  ///////// Interval Timing and Sleep Code ////////////////
  delay(SampleIntSeconds);   // Simple delay for testing only interval set by const in header

  NextAlarmMin = (NextAlarmMin + SampleIntMin) % 60;   // i.e. 65 becomes 5
  rtc.setAlarmMinutes(NextAlarmMin); // RTC time to wake, currently seconds only
  rtc.enableAlarm(rtc.MATCH_MMSS); // Match minutes
  rtc.attachInterrupt(alarmMatch); // Attaches function to be called, currently blank
  delay(50); // Brief delay prior to sleeping not really sure its required
}
void setAlarmHour(){
  ///////// Interval Timing and Sleep Code ////////////////
  delay(SampleIntSeconds);   // Simple delay for testing only interval set by const in header

  NextAlarmHour = (NextAlarmHour + SampleIntHour) % 24;   // i.e. 65 becomes 5
  rtc.setAlarmHours(NextAlarmHour); // RTC time to wake, currently seconds only
  rtc.enableAlarm(rtc.MATCH_HHMMSS); // Match seconds only
  rtc.attachInterrupt(alarmMatch); // Attaches function to be called, currently blank
  delay(50); // Brief delay prior to sleeping not really sure its required
}

// Debbugging output of time/date and battery voltage
void SerialOutput() {

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
void SdOutput() {

  //if (!file.sync() || file.getWriteError()) {
  //  error("write error");
  //  error(3);     // Three red flashes means write failed.
  //}

  // Formatting for file out put dd/mm/yyyy hh:mm:ss, [sensor output]
  logfile.print(rtc.getDay());
  logfile.print("/");
  logfile.print(rtc.getMonth());
  logfile.print("/");
  logfile.print(rtc.getYear()+2000);
  logfile.print(" ");
  logfile.print(rtc.getHours());
  logfile.print(":");
  if(rtc.getMinutes() < 10)
    logfile.print('0');      // Trick to add leading zero for formatting
  logfile.print(rtc.getMinutes());
  logfile.print(":");
  if(rtc.getSeconds() < 10)
    logfile.print('0');      // Trick to add leading zero for formatting
  logfile.print(rtc.getSeconds());
  logfile.print(",");
  logfile.print(pressedButton);   // Print selected button
  logfile.print(",");
  logfile.println(BatteryVoltage ());   // Print battery voltage
}

// Write data header.
void writeHeader() {
  logfile.println("DD/MM/YYYY hh:mm:ss, Button, Battery Voltage");
}

// blink out an error code
void error(uint8_t errno) {
  while(1) {
    uint8_t i;
    for (i=0; i<errno; i++) {
      digitalWrite(RED, HIGH);
      delay(100);
      digitalWrite(RED, LOW);
      delay(100);
    }
    for (i=errno; i<10; i++) {
      delay(200);
    }
  }
}

// blink out an error code, Red on pin #13 or Green on pin #8
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

// Do something when interrupt called
void alarmMatch() {
}

int freeram () {
  char stack_dummy = 0;
  return &stack_dummy - sbrk(0);
}

void playTone(){
  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(SPEAKER, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(8);
  }
}
