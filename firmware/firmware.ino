/*

Example serial message for configuring the radio.
{
  "remoteConfigURL":"http://config.acc-radio.raiotech.com/api/v1/radios/device_interface/v1.0/",
  "remoteConfig":true,
  "radioID":"radioID",
  "hasChannelPot":true,
  "pcbVersion":"pcbVersion",
  "stn1URL":"http://acc-radio.raiotech.com/mansfield.mp3",
  "stn2URL":"https://broadcastify.cdnstream1.com/5974",
  "stn3URL":"https://paineldj6.com.br:20030/stream?type=.mp3",
  "stn4URL":"https://s1-fmt2.liveatc.net/kmfd1_zob04",
  "stationCount":4
}

Example message for resetting the stored preferences and restarting the ESP.
{"clearPreferences": true}

States: 
  - Green               LED_STATUS_SUCCESS              success
  - Green (blinking)    LED_STATUS_SUCCESS_BLINKING     success (blinking)
  - Blue                LED_STATUS_INFO                 info
  - Blue (blinking)     LED_STATUS_INFO_BLINKING        info (blinking)
  - Yellow              LED_STATUS_WARNING              warning
  - Yellow              LED_STATUS_WARNING_BLINKING     warning (blinking)
  - Red                 LED_STATUS_ERROR                error
  - Red (blinking)      LED_STATUS_ERROR_BLINKING       error (blinking)

ESP32-audioI2S Examples/Docs/Source (testing 3.0.0) (Installed manually):
  - https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/src/Audio.h
  - https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/src/Audio.cpp
  - https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/examples/Simple_WiFi_Radio/Simple_WiFi_Radio.ino

WiFiManager (2.0.16-rc.2):
  - https://github.com/tzapu/WiFiManager

ArduinoJSON (6.21.2)
  - https://arduinojson.org/?utm_source=meta&utm_medium=library.properties

TODO:
  - Pull settings from remote server
  - TODO ESP32-audioI2S 3.0.0 has lostStreamDetection!!!
    - This does not exist in 3.0.0 - was the name changed?
    
*/
#define FIRMWARE_VERSION "v0.0"

#include <WiFiManager.h>
#include "Audio.h"
#include "Radio.h"

WiFiManager wifiManager;
Audio audio;
RadioConfig radioConfig;

Radio radio(&radioConfig, &wifiManager, &audio);

// Callbacks which need the radio instance
void audio_info(const char *info) {
  // Triggered once a feed is played.
  if (radio.debugMode) {
    Serial.print("info:");
    Serial.println(info);
  }
}

void wifiManagerSetupCallback(WiFiManager *myWiFiManager) {
  radio.ledStatus.setStatusCode(RADIO_STATUS_300_UNABLE_TO_CONNECT_TO_WIFI_WM_ACTIVE);
  if (radio.debugMode) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.print("Created config portal AP ");
    Serial.println(myWiFiManager->getConfigPortalSSID());
  }
}


void setup() {

  // radio.debugMode = true;

  wifiManager.setAPCallback(wifiManagerSetupCallback);
  radio.init();
}

void loop() {
  radio.loop();
}