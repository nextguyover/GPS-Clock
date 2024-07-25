#define PUSHBUTTON_1 5
#define PUSHBUTTON_2 0

#define DISP_PIN_CS D6

#define HORIZONTAL_DISP_COUNT 4
#define VERTICAL_DISP_COUNT 1

#define GPS_ANIM_INTERVAL 100  // speed to play waiting for GPS fix animation

#define REBOOT_IF_NO_TIME_REFRESH_FOR 7 * 24 * 60 * 60 * 1000  // 7 days
#define REBOOT_PERIOD_START 11                                 // see handleReboots() for more info
#define REBOOT_PERIOD_END 15

#include <TimeLib.h>  // handles timekeeping and timezones
#include <Timezone.h>

#include <TinyGPS++.h>  // libraries for communicating with GPS module
#include <SoftwareSerial.h>

#include <Adafruit_GFX.h>  // libraries for driving LED matrix display
#include <Max72xxPanel.h>

#include <ESP_EEPROM.h>  // EEPROM is used for saving display brightness

TinyGPSPlus gps;          // The TinyGPS++ object
SoftwareSerial ss(4, 5);  // The serial connection to the GPS device  (first param is the important (GPS TX) pin, as
                          // only comms needed with GPS module is receiving data it transmits)

TimeChangeRule StandardTimeRule = {"AEST", First, Sun, Apr, 3, 10 * 60};  // Standard time = UTC + 10 hours
TimeChangeRule DaylightTimeRule = {"AEDT", First, Sun, Oct, 2, 11 * 60};  // Daylight time = UTC + 11 hours
Timezone myTZ(DaylightTimeRule, StandardTimeRule);

Max72xxPanel matrix = Max72xxPanel(DISP_PIN_CS, HORIZONTAL_DISP_COUNT, VERTICAL_DISP_COUNT);

String currentlyDisplayingTime;  // stores the time

bool rebootWhenPossible = false;

const uint64_t IMAGES[] = {  // images for awaiting GPS fix animation
    0x78f0e0c080000000, 0x78e0d0b080000000, 0x78e0c888b0000000, 0x78e0c48488300000, 0x78e0c28082042800,
    0x78e0c18081000228, 0x78e0c08080000302, 0x78e0c08083040808, 0x78e0c08588100010, 0x78e0ca90a0002000,
    0x78eae080c0004000, 0x7ae0c08080008000, 0x78e0c08080000000, 0x78e0c08080000000, 0x78e0c08080000000,
    0x78e0c08080000000, 0x78e0c08080000000, 0x78e0c08080000000};

unsigned long animWaitUntil = 0;
int imgIndex = 0;

unsigned long timeRefreshAge = 0;

bool pushbutton1PrevState = false;
bool pushbutton2PrevState = false;

int currentBrightness = 0;

unsigned long brightnessSaveTime = 0;

void setup() {
    Serial.begin(115200);
    ss.begin(9600);

    pinMode(PUSHBUTTON_1, INPUT_PULLUP);
    pinMode(PUSHBUTTON_2, INPUT_PULLUP);

    EEPROM.begin(16);
    EEPROM.get(0, currentBrightness);

    matrix.setIntensity(currentBrightness);  // Use a value between 0 and 15 for brightness
    matrix.setRotation(0, 1);                // Rotate the display 90 degrees
    matrix.setRotation(1, 1);
    matrix.setRotation(2, 1);
    matrix.setRotation(3, 1);
    matrix.fillScreen(LOW);
    matrix.write();
}

void loop() {
    while (ss.available() > 0) {       // while data is available
        if (gps.encode(ss.read())) {   // read gps data
            if (gps.time.isValid()) {  // check whether gps date is valid
                if (gps.satellites.value() > 0) {
                    timeRefreshAge = millis();
                    rebootWhenPossible = false;
                    setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(),
                            gps.date.year());
                }
            }
        }
    }

    handleLCD();

    handleReboots();

    handleBrightnessChange();

    handleBrightnessSave();
}

void handleReboots() {
    if ((unsigned long)(millis() - timeRefreshAge) >= REBOOT_IF_NO_TIME_REFRESH_FOR) {  // if time hasn't been refreshed
                                                                                        // in the given duration, reboot
                                                                                        // ESP when possible
        rebootWhenPossible = true;  // this is because a non-working clock is preferable to displaying incorrect time
    }

    if (rebootWhenPossible) {
        time_t local = myTZ.toLocal(now());

        if (hour(local) > REBOOT_PERIOD_START and hour(local) < REBOOT_PERIOD_END) {  // reboot if needed between set
                                                                                      // period
            ESP.restart();
        }
    }
}

void handleLCD() {
    if (timeStatus() == timeNotSet) {
        unsigned long millisSnapshot = millis();
        if ((unsigned long)(millisSnapshot - animWaitUntil) >= GPS_ANIM_INTERVAL) {
            matrix.fillScreen(LOW);

            matrix.drawChar(1, 1, 'G', HIGH, LOW, 1);
            matrix.drawChar(9, 1, 'P', HIGH, LOW, 1);
            matrix.drawChar(17, 1, 'S', HIGH, LOW, 1);

            unsigned char glyph[sizeof(IMAGES[imgIndex])];
            memcpy(glyph, &IMAGES[imgIndex], sizeof(IMAGES[imgIndex]));

            matrix.drawBitmap(24, 0, glyph, 8, 8, 1);

            matrix.write();  // Send bitmap to display

            imgIndex++;

            if (imgIndex > 17) {
                imgIndex = 0;
            }

            animWaitUntil = millisSnapshot;
        }
    }

    time_t local = myTZ.toLocal(now());

    if (currentlyDisplayingTime != formattedTimeForLCD(local) && timeStatus() == timeSet) {
        matrix.fillScreen(LOW);

        char currentTimeCharArr[8];
        formattedTimeForLCD(local).toCharArray(currentTimeCharArr, 9);

        matrix.drawChar(2, 0, currentTimeCharArr[0], HIGH, LOW, 1);  // H
        matrix.drawChar(9, 0, currentTimeCharArr[1], HIGH, LOW, 1);  // HH

        matrix.drawPixel(15, 2, 1);
        matrix.drawPixel(15, 4, 1);

        matrix.drawChar(17, 0, currentTimeCharArr[3], HIGH, LOW, 1);  // HH:M
        matrix.drawChar(24, 0, currentTimeCharArr[4], HIGH, LOW, 1);  // HH:MM

        matrix.write();  // Send bitmap to display
        Serial.println("Printed time " + (String)formattedTimeForLCD(local) + " to display");

        currentlyDisplayingTime = formattedTimeForLCD(local);
    }
}

String formattedTimeForLCD(time_t t) {
    String currentTime =
        (((String)hourFormat12(t)).length() == 2 ? (String)hourFormat12(t) : "0" + (String)hourFormat12(t));
    currentTime += ":";
    currentTime += (((String)minute(t)).length() == 2 ? (String)minute(t) : "0" + (String)minute(t));
    currentTime += ":";
    currentTime += (((String)second(t)).length() == 2 ? (String)second(t) : "0" + (String)second(t));

    return currentTime;
}

void handleBrightnessChange() {
    bool pushbutton1State = !digitalRead(PUSHBUTTON_1);
    bool pushbutton2State = !digitalRead(PUSHBUTTON_2);

    if (pushbutton1State != pushbutton1PrevState) {
        if (pushbutton1State) {
            currentBrightness = constrain(currentBrightness + 1, 0, 15);
            matrix.setIntensity(currentBrightness);
            matrix.write();
        }
        pushbutton1PrevState = pushbutton1State;
    }

    if (pushbutton2State != pushbutton2PrevState) {
        if (pushbutton2State) {
            currentBrightness = constrain(currentBrightness - 1, 0, 15);
            matrix.setIntensity(currentBrightness);
            matrix.write();
        }
        pushbutton2PrevState = pushbutton2State;
    }
}

void handleBrightnessSave() {
    unsigned long currentMillis = millis();
    if ((unsigned long)(currentMillis - brightnessSaveTime) >= 60000) {
        int eepromData = 0;
        EEPROM.get(0, eepromData);

        if (currentBrightness != eepromData) {
            EEPROM.put(0, currentBrightness);

            if (!EEPROM.commit()) {
                Serial.println("EEPROM commit failed!");
            }
        }

        brightnessSaveTime = currentMillis;
    }
}