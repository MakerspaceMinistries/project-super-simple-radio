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

Audio.h Examples/Docs/Source:
  - https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/src/Audio.h
  - https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/src/Audio.cpp
  - https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/examples/Simple_WiFi_Radio/Simple_WiFi_Radio.ino

TODO:
  - Pull settings from remote server

  Audio/DAC
  - Make sure it's using PSRAM, document how this is done
  - Attempt to detect disconnects and set LED status

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

#define VOLUME_CHANNEL_READ_INTERVAL_MS 50

#define CURRENT_TIME_CHECK_INTERVAL_S 3

WiFiManager wifiManager;
Audio audio;


unsigned long lastCurrentTimeCheck = 0;
uint32_t lastCurrentTime = 0;

Radio radio(PIN_CHANNEL_POT, PIN_VOLUME_POT, VOLUME_MIN, VOLUME_MAX, &wifiManager);

unsigned long lastVolumeChannelRead = 0;
// TODO lastMinuteInterval should have a better name, or the function that uses it should.
unsigned long lastMinuteInterval = 0;

void configModeCallback(WiFiManager *myWiFiManager) {
  radio.ledStatus.setStatus(LED_STATUS_WARNING_BLINKING);
  if (radio.debugMode) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.print("Created config portal AP ");
    Serial.println(myWiFiManager->getConfigPortalSSID());
  }
}

void checkWiFiDisconnect() {
  // Check every minute if the wifi connection has been lost and reconnect.
  if (millis() > lastMinuteInterval + 60 * 1000) {
    if (radio.debugMode) Serial.println("Checking WiFi connection...");
    if (WiFi.status() != WL_CONNECTED) {
      if (radio.debugMode) Serial.println("Reconnecting to WiFi...");
      radio.ledStatus.setStatus(LED_STATUS_WARNING_BLINKING);
      WiFi.disconnect();
      WiFi.reconnect();
    }
    // Reset interval
    lastMinuteInterval = millis();
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

  // This should be run first to make sure that debug mode is set right away.
  radio.init();
  radio.ledStatus.init();
  // radio.debugMode = true;

  if (radio.debugMode) {
    Serial.begin(115200);
    Serial.println("DEBUG MODE ON");
    radio.ledStatus.setStatus(LED_STATUS_SUCCESS);
    delay(100);
    radio.ledStatus.setStatus(LED_STATUS_INFO);
    radio.debugPrintConfigToSerial();
  }

  radio.ledStatus.setStatus(LED_STATUS_FADE_SUCCESS_TO_INFO);
  radio.ledStatus.setStatus(LED_STATUS_INFO);

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
  // Check if the WiFi has disconnected
  checkWiFiDisconnect();
  audio.loop();
  radio.loop();

  // Move the logic below to radio.loop(), and use return to make sure the ledStatus is correct, and refactor if's to be less deep


  // Check wifi connection, if lost, set status and return; so that the radio doesn't overwrite the status
  // radio.loop()

  if (millis() > lastVolumeChannelRead + VOLUME_CHANNEL_READ_INTERVAL_MS) {

    // Read the current state of the radio's inputs
    int selectedVolume = radio.readVolume();
    int selectedChannelIdx = radio.readChannelIdx();

    // Setting volume doesn't use extra resources, do it even if there were no changes
    audio.setVolume(selectedVolume);

    /*

    Check if the requested status is different from the current status, if different, make changes
    
    */

    if (selectedChannelIdx != radio.status.channelIdx) {
      // change channel, set radio.status.playing to false to trigger a reconnect.
      radio.status.channelIdx = selectedChannelIdx;
      radio.status.playing = false;
    }

    if (selectedVolume > 0 && !radio.status.playing) {

      // start play, set radio.status.playing
      radio.ledStatus.setStatus(LED_STATUS_INFO_BLINKING);

      String channels[] = { radio.stn1URL, radio.stn2URL, radio.stn3URL, radio.stn4URL };
      char selectedChannelURL[2048];
      channels[radio.status.channelIdx].toCharArray(selectedChannelURL, 2048);
      radio.status.playing = audio.connecttohost(selectedChannelURL);

      // This may overwrite an error status since it doesn't rely on a status change, but fires every VOLUME_CHANNEL_READ_INTERVAL_MS
      if (radio.status.playing) {
        radio.ledStatus.setStatus(LED_STATUS_SUCCESS);
      } else {
        radio.ledStatus.setStatus(LED_STATUS_WARNING_BLINKING);
      }

      if (radio.debugMode) {
        Serial.print("selectedChannelURL=");
        Serial.println(selectedChannelURL);
      }
    }

    if (selectedVolume == 0 && radio.status.playing) {
      // stop play, set status
      radio.status.playing = false;
      audio.stopSong();
      radio.ledStatus.setStatus(LED_STATUS_LIGHTS_OFF);
    }

    // Check if the audio.getAudioCurrentTime() is advancing, if not, set the status to match (triggering a reconnect).
    if (radio.status.playing && millis() > lastCurrentTimeCheck + CURRENT_TIME_CHECK_INTERVAL_S * 1000) {
      lastCurrentTimeCheck = millis();
      uint32_t currentTime = audio.getAudioCurrentTime();
      if (lastCurrentTime - currentTime > CURRENT_TIME_CHECK_INTERVAL_S * 0.9) {
        radio.status.playing = false;
      }
      lastCurrentTime = currentTime;
    }
  }
}