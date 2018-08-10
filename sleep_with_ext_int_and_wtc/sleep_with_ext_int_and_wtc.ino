#include <RTCZero.h>

/* Create an rtc object */
RTCZero rtc;
int AlarmTime;

const byte ledPin = 13;
const byte interruptPin = A1;
enum stateType {
  SLEEP = 0,
  WAKE_BY_PIN,
  WAKE_BY_RTC,
  RUNNING
};
volatile byte state = SLEEP;

void setup() {
  rtc.begin();
  
  pinMode(ledPin, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
}

void loop() {
  if (state == SLEEP) {
    go_sleep();
  }
  else if (state == WAKE_BY_PIN) {
    detach_all();
    blink(ledPin, 5);
    go_sleep();
  }
  else if (state == WAKE_BY_RTC) {
    detach_all();
    blink(ledPin, 2);
    go_sleep();
  }
  else {
    // running
  }
}

void go_sleep() {
    attachInterrupt(digitalPinToInterrupt(interruptPin), pin_isr, LOW);

    AlarmTime = rtc.getSeconds()+10; // Adds 10 seconds to alarm time
    AlarmTime = AlarmTime % 60; // checks for roll over 60 seconds and corrects
    
    rtc.setAlarmSeconds(AlarmTime); // Wakes at next alarm time
    rtc.enableAlarm(rtc.MATCH_SS); // Match seconds only
    rtc.attachInterrupt(rtc_isr);
    
    rtc.standbyMode();    // Sleep until next alarm match
    //standby();
}

void detach_all() {
    detachInterrupt(digitalPinToInterrupt(interruptPin));
    rtc.detachInterrupt();
    rtc.disableAlarm();
}

void pin_isr() {
  state = WAKE_BY_PIN;
}

void rtc_isr() {
  state = WAKE_BY_RTC;
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

void  standby() {
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
  __DSB();
  __WFI();
}
