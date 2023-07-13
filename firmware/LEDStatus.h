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

Status Codes:

  Status codes (in the form of levels and codes) can be sent instead of LED Statuses. These codes are then interpreted into LED Statuses, and the status code is also stored.

  Each status status code is stored individually by level, but only one level is displayed.

  Codes are hierarchical in when/how they are displayed, in that an error code will take priority in being displayed over a success code. Likewise, a success code will have priority over an info code.

  If a higher priority status code is cleared, a previously set lower priority status code will then become active.

  -  -1     Unset (Idle)
  -   0- 49 Info
  -  50- 99 Info (Blinking)
  - 100-149 Success
  - 150-199 Success (Blinking)
  - 200-249 Warning
  - 250-299 Warning (Blinking)
  - 300-349 Error
  - 350-399 Error (Blinking)

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

#define LED_STATUS_DEFAULT_PIN_LED_RED 4
#define LED_STATUS_DEFAULT_PIN_LED_GREEN 5
#define LED_STATUS_DEFAULT_PIN_LED_BLUE 6
#define LED_STATUS_DEFAULT_LED_ON LOW
#define LED_STATUS_DEFAULT_LED_OFF HIGH

#define LED_STATUS_DEFAULT_TIMER_NUMBER 0
#define LED_STATUS_DEFAULT_TIMER_DIVIDER 80
#define LED_STATUS_DEFAULT_TIMER_ALARM_VALUE 600000

#define LED_STATUS_UNSET_STATUS_CODE -1

#define LED_STATUS_300_ERROR_LEVEL 3
#define LED_STATUS_200_WARNING_LEVEL 2
#define LED_STATUS_100_SUCCESS_LEVEL 1
#define LED_STATUS_000_INFO_LEVEL 0

#define LED_STATUS_INFO_LIGHTS_OFF_CODE 0

// The callback and variables required by the callback are global for simplicity's sake.
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
  bool mDebugMode;
  int mLedOff;
  int mLedOn;
  int mTimerNumber;
  int mTimerDivider;
  int mTimerAlarmValue;
  int mStatusCodes[4] = { LED_STATUS_UNSET_STATUS_CODE, LED_STATUS_UNSET_STATUS_CODE, LED_STATUS_UNSET_STATUS_CODE, LED_STATUS_UNSET_STATUS_CODE };
  int mActiveStatusCode = 0;
  int gLedStatusRGBActive[3] = { 0, 0, 0 };
  hw_timer_t *mTimerBlink = NULL;
  void setRGBActive(int r, int g, int b);
  void timerEnable();
  void timerDisable();
  void writeRGB(int r, int g, int b);
  void computeActiveStatusCode(int blockingDelay);
  int getIndexOfActiveStatus();
  int levelCode2StatusCode(int level, int code);
  bool isBlinkingCode(int code);

public:
  LEDStatus();
  void configure(int rPin, int gPin, int bPin, int ledOff, int ledOn, int timerNumber, int timerDivider, int timerAlarmValue);
  void setRGB(int r, int g, int b);
  void setStatus(int status);
  void setStatusCode(int statusCode, int blockingDelay);
  void setStatusCode(int level, int code, int blockingDelay);
  void clearStatusLevel(int level);
  void clearAllStatusCodesTo(int level);
  int getActiveStatusCode();
  void init(bool debugMode);
};

LEDStatus::LEDStatus() {
  // This should be refactored along with the configure method. Make a LEDStatusConfig struct
  mLedOn = LED_STATUS_DEFAULT_LED_ON;
  mLedOff = LED_STATUS_DEFAULT_LED_OFF;
  mTimerNumber = LED_STATUS_DEFAULT_TIMER_NUMBER;
  mTimerDivider = LED_STATUS_DEFAULT_TIMER_DIVIDER;
  mTimerAlarmValue = LED_STATUS_DEFAULT_TIMER_ALARM_VALUE;
  gLedStatusRGBPins[0] = LED_STATUS_DEFAULT_PIN_LED_RED;
  gLedStatusRGBPins[1] = LED_STATUS_DEFAULT_PIN_LED_GREEN;
  gLedStatusRGBPins[2] = LED_STATUS_DEFAULT_PIN_LED_BLUE;
}

void LEDStatus::configure(int rPin, int gPin, int bPin, int ledOff = LED_STATUS_DEFAULT_LED_OFF, int ledOn = LED_STATUS_DEFAULT_LED_ON, int timerNumber = LED_STATUS_DEFAULT_TIMER_NUMBER, int timerDivider = LED_STATUS_DEFAULT_TIMER_DIVIDER, int timerAlarmValue = LED_STATUS_DEFAULT_TIMER_ALARM_VALUE) {
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

void LEDStatus::init(bool debugMode = false) {
  mDebugMode = debugMode;
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

int LEDStatus::getIndexOfActiveStatus() {
  int idx = 0;

  // No need to compare the 0th to itself, start at 1
  for (int i = 1; i < 4; i++) {

    // If the value is unset, then no need to do anything, otherwise compute code based using the index and compare
    if (mStatusCodes[i] != LED_STATUS_UNSET_STATUS_CODE && levelCode2StatusCode(i, mStatusCodes[i]) > levelCode2StatusCode(idx, mStatusCodes[idx])) {
      idx = i;
    }
  }
  return idx;
}

void LEDStatus::setStatusCode(int statusCode, int blockingDelay = 0) {
  int level = statusCode / 100;
  int code = statusCode - level * 100;
  setStatusCode(level, code, blockingDelay);
}

void LEDStatus::setStatusCode(int level, int code, int blockingDelay) {
  /*
  returns true if status code is changed

  Using blockingDelay sparingly, as it blocks ;)
  */

  if (code == mStatusCodes[level]) { return; }
  mStatusCodes[level] = code;
  computeActiveStatusCode(blockingDelay);
  if (mDebugMode) {
    Serial.print("Request to set Status Code to: ");
    Serial.println(levelCode2StatusCode(level, code));
    Serial.print("Active Status Code: ");
    Serial.println(mActiveStatusCode);
  }
}

void LEDStatus::clearStatusLevel(int level) {
  return setStatusCode(level, LED_STATUS_UNSET_STATUS_CODE, 0);
}

void LEDStatus::clearAllStatusCodesTo(int level = 0) {
  for (int i = level; i >= 0; i--) {
    mStatusCodes[i] = LED_STATUS_UNSET_STATUS_CODE;
  }
}

int LEDStatus::getActiveStatusCode() {
  return mActiveStatusCode;
}

int LEDStatus::levelCode2StatusCode(int level, int code) {
  return level * 100 + code;
}

bool LEDStatus::isBlinkingCode(int code) {
  return (code > 49);
}

void LEDStatus::computeActiveStatusCode(int blockingDelay = 0) {
  int level = getIndexOfActiveStatus();
  int code = mStatusCodes[level];
  int statusCode = levelCode2StatusCode(level, code);
  mActiveStatusCode = statusCode;

  bool blinking = isBlinkingCode(code);
  switch (level) {
    case 0:  // Info
      if (code == LED_STATUS_INFO_LIGHTS_OFF_CODE) {
        setStatus(LED_STATUS_LIGHTS_OFF);
      } else if (blinking) {
        setStatus(LED_STATUS_INFO_BLINKING);
      } else {
        setStatus(LED_STATUS_INFO);
      }
      break;
    case 1:  // Success
      if (blinking) {
        setStatus(LED_STATUS_SUCCESS_BLINKING);
      } else {
        setStatus(LED_STATUS_SUCCESS);
      }
      break;
    case 2:  // Warning
      if (blinking) {
        setStatus(LED_STATUS_WARNING_BLINKING);
      } else {
        setStatus(LED_STATUS_WARNING);
      }
      break;
    case 3:  // Error
      if (blinking) {
        setStatus(LED_STATUS_ERROR_BLINKING);
      } else {
        setStatus(LED_STATUS_ERROR);
      }
      break;
  }

  if (blockingDelay > 0) {
    delay(blockingDelay);
  }
}
