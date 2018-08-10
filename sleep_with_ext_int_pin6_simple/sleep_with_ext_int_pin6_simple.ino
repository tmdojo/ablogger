const byte ledPin = 13;
const byte interruptPin = A1;
volatile byte state = LOW;

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, LOW);
}

void loop() {
  digitalWrite(ledPin, state);
}

void blink() {
  state = !state;
}
