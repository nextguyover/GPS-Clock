# GPS Clock

This repository contains firmware for the ESP8266 (Wemos D1 Mini dev board) for a simple LED matrix clock that uses GPS as the time source. See [this blogpost](https://insertnewline.com/blog/gps-clock) for more info.

# Usage

To flash this firmware open `GPS_clock.ino` in the Arduino IDE, [install ESP8266 Core](https://arduino-esp8266.readthedocs.io/en/latest/installing.html), then install the [required libraries](#required-libraries), then compile and flash. 

## Setting Timezone

Before flashing the firmware, set the timezone for your location by setting the `StandardTimeRule` and `DaylightTimeRule` variables in `GPS_clock.ino`. Follow the [instructions provided by the Timezone library](https://github.com/JChristensen/Timezone) to do this.

## Required Libraries

- [TinyGPS++](https://github.com/mikalhart/TinyGPSPlus)
- [Time](https://www.arduino.cc/reference/en/libraries/time/)
- [Timezone](https://www.arduino.cc/reference/en/libraries/timezone/)
- [Adafruit GFX](https://www.arduino.cc/reference/en/libraries/adafruit-gfx-library/)
- [Max72xxPanel](https://github.com/markruys/arduino-Max72xxPanel)
- [ESP_EEPROM](https://www.arduino.cc/reference/en/libraries/esp_eeprom/)

## EEPROM Commit Errors

If you see errors in the Serial console that EEPROM commit failed, this is due to an issue in the ESP_EEPROM library. See [this GitHub issue](https://github.com/jwrw/ESP_EEPROM/issues/12#issuecomment-2247661104) and implement the fix, then try again. If this is unsuccessful, replace `#include <ESP_EEPROM.h>` with `#include <EEPROM.h>`.