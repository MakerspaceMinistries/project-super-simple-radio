// https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/timer.html

/*
Status Options:
  - Special (Blocking)  LED_STATUS_FADE_SUCCESS_TO_INFO
  - Lights Off          LED_STATUS_LIGHTS_OFF           
  - Green               LED_STATUS_SUCCESS              success
  - Green (blinking)    LED_STATUS_SUCCESS_BLINKING     success (blinking)
  - Blue                LED_STATUS_INFO                 info
  - Blue (blinking)     LED_STATUS_INFO_BLINKING        info (blinking)
  - Yellow              LED_STATUS_WARNING              warning
  - Yellow              LED_STATUS_WARNING_BLINKING     warning (blinking)
  - Red                 LED_STATUS_ERROR                error
  - Red (blinking)      LED_STATUS_ERROR_BLINKING       error (blinking)

TODO:

This in the switch should be made into a function:

  writeRGB(mLedOff, mLedOn, mLedOff);
  setRGBActive(0, 1, 0);
  timerEnable();

*/

#include "esp_system.h"

#define LED_STATUS_FADE_SUCCESS_TO_INFO_ITERATIONS 1000
#define LED_STATUS_FADE_SUCCESS_TO_INFO 0
#define LED_STATUS_LIGHTS_OFF 1
#define LED_STATUS_SUCCESS 2
#define LED_STATUS_SUCCESS_BLINKING 3
#define LED_STATUS_INFO 4
#define LED_STATUS_INFO_BLINKING 5
#define LED_STATUS_WARNING 6
#define LED_STATUS_WARNING_BLINKING 7
#define LED_STATUS_ERROR 8
#define LED_STATUS_ERROR_BLINKING 9

// The callback and variables required by the callback are global for simplicities sake.
// This explains how to get around that: https://arduino.stackexchange.com/a/89176
int gLedStatusRGBPins[3]{ 0, 0, 0 };
int gLedStatusRGBActive[3] = { 0, 0, 0 };
void timerCallback() {
  // DO NOT change any variables here (that are from outside this scope)
  for (int i; i < 3; i++) {
    if (gLedStatusRGBActive[i] == 1) {
      int state = digitalRead(gLedStatusRGBPins[i]);
      digitalWrite(gLedStatusRGBPins[i], !state);
    }
  }
}

class LEDStatus {
  int mLedOff;
  int mLedOn;
  int mTimerNumber;
  int mTimerDivider;
  int mTimerAlarmValue;
  int gLedStatusRGBActive[3] = { 0, 0, 0 };
  hw_timer_t *mTimerBlink = NULL;
  void setRGBActive(int r, int g, int b);
  void timerEnable();
  void timerDisable();
  void writeRGB(int r, int g, int b);

public:
  LEDStatus(int rPin, int gPin, int bPin, int ledOff, int ledOn, int timerNumber, int mTimerDivider, int mTimerAlarmValue);
  void setRGB(int r, int g, int b);
  void setStatus(int status);
  void init();
};

LEDStatus::LEDStatus(int rPin, int gPin, int bPin, int ledOff = 0, int ledOn = 1, int timerNumber = 0, int timerDivider = 80, int timerAlarmValue = 600000) {
  mLedOn = ledOn;
  mLedOff = ledOff;
  mTimerNumber = timerNumber;
  mTimerDivider = timerDivider;
  mTimerAlarmValue = timerAlarmValue;
  gLedStatusRGBPins[0] = rPin;
  gLedStatusRGBPins[1] = gPin;
  gLedStatusRGBPins[2] = bPin;
}

void LEDStatus::setRGBActive(int r, int g, int b) {
  gLedStatusRGBActive[0] = r;
  gLedStatusRGBActive[1] = g;
  gLedStatusRGBActive[2] = b;
}

void LEDStatus::timerEnable() {
  timerAlarmEnable(mTimerBlink);
}

void LEDStatus::timerDisable() {
  timerAlarmDisable(mTimerBlink);
}

void LEDStatus::init() {
  pinMode(gLedStatusRGBPins[0], OUTPUT);
  pinMode(gLedStatusRGBPins[1], OUTPUT);
  pinMode(gLedStatusRGBPins[2], OUTPUT);
  writeRGB(mLedOff, mLedOff, mLedOff);
  mTimerBlink = timerBegin(mTimerNumber, mTimerDivider, true);
  timerAttachInterrupt(mTimerBlink, timerCallback, true);
  timerAlarmWrite(mTimerBlink, mTimerAlarmValue, true);
}

void LEDStatus::setStatus(int status) {
  writeRGB(mLedOff, mLedOff, mLedOff);
  timerDisable();
  switch (status) {
    case LED_STATUS_FADE_SUCCESS_TO_INFO:
      writeRGB(mLedOff, mLedOn, mLedOff);
      delay(250);
      for (int i = 0; i < LED_STATUS_FADE_SUCCESS_TO_INFO_ITERATIONS; i++) {
        writeRGB(mLedOff, mLedOn, mLedOff);
        delayMicroseconds(LED_STATUS_FADE_SUCCESS_TO_INFO_ITERATIONS - i);
        writeRGB(mLedOff, mLedOff, mLedOn);
        delayMicroseconds(i);
      }
      break;
    case LED_STATUS_LIGHTS_OFF:
      // This is taken care of above.
      break;
    case LED_STATUS_SUCCESS:
      writeRGB(mLedOff, mLedOn, mLedOff);
      break;
    case LED_STATUS_SUCCESS_BLINKING:
      writeRGB(mLedOff, mLedOn, mLedOff);
      setRGBActive(0, 1, 0);
      timerEnable();
      break;
    case LED_STATUS_INFO:
      writeRGB(mLedOff, mLedOff, mLedOn);
      break;
    case LED_STATUS_INFO_BLINKING:
      writeRGB(mLedOff, mLedOff, mLedOn);
      setRGBActive(0, 0, 1);
      timerEnable();
      break;
    case LED_STATUS_WARNING:
      writeRGB(mLedOn, mLedOn, mLedOff);
      break;
    case LED_STATUS_WARNING_BLINKING:
      writeRGB(mLedOn, mLedOn, mLedOff);
      setRGBActive(1, 1, 0);
      timerEnable();
      break;
    case LED_STATUS_ERROR:
      writeRGB(mLedOn, mLedOff, mLedOff);
      break;
    case LED_STATUS_ERROR_BLINKING:
      writeRGB(mLedOn, mLedOff, mLedOff);
      setRGBActive(1, 0, 0);
      timerEnable();
      break;
  }
}

void LEDStatus::writeRGB(int r, int g, int b) {
  digitalWrite(gLedStatusRGBPins[0], r);
  digitalWrite(gLedStatusRGBPins[1], g);
  digitalWrite(gLedStatusRGBPins[2], b);
}