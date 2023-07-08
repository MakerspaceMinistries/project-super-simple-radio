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

ESP32-audioI2S Examples/Docs/Source (2.0.0, testing 3.0.0) (Installed manually):
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

  Audio/DAC
  - Make sure it's using PSRAM, document how this is done

*/
#define FIRMWARE_VERSION "v0.0"

#include <WiFiManager.h>
#include "Audio.h"
#include "Radio.h"

#define PIN_CHANNEL_POT 2
#define PIN_VOLUME_POT 8
#define PIN_DAC_SD_MODE 11
#define PIN_I2S_DOUT 12
#define PIN_I2S_BCLK 13
#define PIN_I2S_LRC 14
// TODO: LED Status settings should be passed to the Radio, and then on to the LEDStatus

#define VOLUME_MIN 0
#define VOLUME_MAX 21

WiFiManager wifiManager;
Audio audio;


// make a config struct, with default values. radio(radioConfig, &wifiManager, &audio)
Radio radio(PIN_CHANNEL_POT, PIN_VOLUME_POT, VOLUME_MIN, VOLUME_MAX, &wifiManager, &audio);




// TODO these should at least fire a method, not handle it here. 
void configModeCallback(WiFiManager *myWiFiManager) {
  radio.ledStatus.setStatus(LED_STATUS_WARNING_BLINKING);
  if (radio.debugMode) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.print("Created config portal AP ");
    Serial.println(myWiFiManager->getConfigPortalSSID());
  }
}

void audio_info(const char *info) {
  // Triggered once a feed is played.
  if (radio.debugMode) {
    Serial.print("info:");
    Serial.println(info);
  }
}




void setup() {

  // Consider moving this to the Radio object
  pinMode(PIN_DAC_SD_MODE, OUTPUT);

  // Turn DAC on
  digitalWrite(PIN_DAC_SD_MODE, HIGH);

  // radio.debugMode = true;

  // This should be run first to make sure that debug mode is set right away.
  radio.init();

  // WiFiManager
  WiFi.mode(WIFI_STA);
  wifiManager.setDebugOutput(radio.debugMode);
  wifiManager.setAPCallback(configModeCallback);
  bool res;
  res = wifiManager.autoConnect("Radio Setup");
  if (!res) {
    if (radio.debugMode) Serial.println("Failed to connect or hit timeout");
    // This is a hard stop
    radio.ledStatus.setStatus(LED_STATUS_ERROR_BLINKING);
    delay(5000);
    ESP.restart();
  }

  audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  audio.setVolume(0);

  // Turn lights off if it connects to the remote list server, or does not need to.
  radio.ledStatus.setStatus(LED_STATUS_LIGHTS_OFF);

  // Get stations from config server
  bool error = radio.getConfigFromRemote();
  if (error) {
    radio.ledStatus.setStatus(LED_STATUS_WARNING_BLINKING);
    delay(5000);
  }
}

void loop() {
  audio.loop();
  radio.loop();
}