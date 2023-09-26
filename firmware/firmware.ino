/*

Example serial messages for configuring the radio.

{
  "remoteConfigURL":"http://config.acc-radio.raiotech.com/api/v1/radios/device_interface/v1.0/",
  "radioID":"radioID",
  "pcbVersion":"v1-USMX.beta-1",
  "remoteConfig":true,
  "hasChannelPot":true,
  "stationCount":4
}

It appears that it's too large to send all at once, so it is broken into parts.

{
  "stn1URL":"http://acc-radio.raiotech.com/connect-test.mp3",
  "stn2URL":"http://acc-radio.raiotech.com/connect-test.mp3",
  "stn3URL":"http://acc-radio.raiotech.com/connect-test.mp3",
  "stn4URL":"http://acc-radio.raiotech.com/connect-test.mp3"
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

Board:
  esp32 2.0.9
    IMPORTANT 2.0.13 Doesn't work. It causes an exception when connecting to a stream:
      Decoding stack results
      0x42013a7c:  is in WiFiClient::write(unsigned char const*, unsigned int) (/home/jake/.arduino15/packages/esp32/hardware/esp32/2.0.9/libraries/WiFi/src/WiFiClient.cpp:400).
      0x42013b85: WiFiClient::connected() at /home/jake/.arduino15/packages/esp32/hardware/esp32/2.0.9/libraries/WiFi/src/WiFiClient.cpp:535
      0x42022691: Audio::setDefaults() at /home/jake/Arduino/libraries/ESP32-audioI2S-3.0.0/src/Audio.cpp:137
      0x42022b12:  is in Audio::connecttohost(char const*, char const*, char const*) (/home/jake/Arduino/libraries/ESP32-audioI2S-3.0.0/src/Audio.cpp:412).
      0x4200578e:  is in Radio::Radio(RadioConfig*, WiFiManager*, Audio*) (/home/jake/repos/project-super-simple-radio/firmware/LEDStatus.h:132).
      0x4200a30c: Radio::loop() at /home/jake/repos/project-super-simple-radio/firmware/Radio.h:467
      0x4200a34a: Radio::loop() at /home/jake/repos/project-super-simple-radio/firmware/Radio.h:510
      0x4203c4c9: rmt_set_tx_thr_intr_en at /Users/ficeto/Desktop/ESP32/ESP32S2/esp-idf-public/components/hal/esp32s3/include/hal/rmt_ll.h:329

ESP32-audioI2S Examples/Docs/Source (testing 3.0.0) (Installed manually):
  - https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/src/Audio.h
  - https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/src/Audio.cpp
  - https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/examples/Simple_WiFi_Radio/Simple_WiFi_Radio.ino

WiFiManager (2.0.16-rc.2):
  - https://github.com/tzapu/WiFiManager

ArduinoJSON (6.21.2)
  - https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
    
*/
#define FIRMWARE_VERSION "v1.0.0-beta.1"

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