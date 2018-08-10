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

## for v0.1

| Feather | component | Note |
| A0 | Piezo | The other leg is connected to GND |
| 11 | Button A | The other leg is connected to GND |
| 12 | Button B | The other leg is connected to GND |
| 10 | Slide switch | Middle leg is connected to GND and the other left floating |
| 4 | SDCS | CS for SD card of Adafruit Adalogger M0 |

## for v1.0 PCB

| Feather | component | Note |
| A0 | Piezo | The other leg is connected to GND |
| 11 | Button A | The other leg is connected to GND |
| 12 | Button B | The other leg is connected to GND |
| 10 | Slide switch | Middle leg is connected to GND and the other left floating |
| 5 | SDCS | CS for SD card of adafruit Adalogger FeatherWing|
| 9(A7) | battery monitor | Feather buit-in voltage divider |
| 6 | RTC INT | RTC INT from adafruit Adalogger FeatherWing <-- Not used|


## for v1.1 PCB (to be designed)

| Feather | component | Note |
| A0 | Piezo | The other leg is connected to GND |
| 11 | Button A | The other leg is connected to GND |
| 12 | Button B | The other leg is connected to GND |
| 5 | Slide switch | Middle leg is connected to GND and the other left floating |
| 10 | SDCS | CS for SD card of adafruit Adalogger FeatherWing |
| 9 | battery monitor | Feather buit-in voltage divider |
| 6 | RTC INT | RTC INT from adafruit Adalogger FeatherWing|

Note that pins for SDCS and Slide switch is swapped from v1.0 (to match default of Adafruit Adalogger FeatherWing)
