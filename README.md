# ablogger
Log A/B survey at certain interval

# Description

This is a tool to record a person's feelings at one hour interval.
The aim is to collect emotional labels which are to be studied together with data from sensors such as health tracker.

The device would beep at certain interval, say every hour, prompting user to push A or B button. Upon pressing a button, it records the button with time stamp.

# Hardware components

* Adafruit Feather M0 Adaloger:
* piezo buzzer: play sound for input queue
* two push buttons: Select A or B
* slide switch: Turn on/off sound
* uSD card: Record in csv file

# Schematics

To be added...

| Feather | component | Note |
| A0 | Piezo | The other leg is connected to GND |
| 11 | Button A | The other leg is connected to GND |
| 12 | Button B | The other leg is connected to GND |
| 10 | Slide switch | Middle leg is connected to GND and the other left floating |
