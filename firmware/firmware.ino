/*

Example serial message for configuring the radio.
{ "remoteListURL": "remoteListURL", "remoteList": true, "radioID":"radioID", "hasChannelPot":true, "pcbVersion":"pcbVersion", "stnOneURL":"stnOneURL", "stnTwoURL":"stnTwoURL", "stnThreeURL":"stnThreeURL", "stationCount":2, "maxStationCount":3}

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
  - Refactor setDebugMode
  - Number of channels it supports in the map doesn't match the number of channels available to set in the channelMap

  Audio/DAC
  - Make sure it's using PSRAM, document how this is done
  - Handle DAC enable (pin 11) (set high to turn DAC on, low to turn DAC off)
  - Attempt to detect disconnects and set LED status

*/

#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include "Audio.h"
#include "LEDStatus.h"
#include "ChannelPot.h"

#define FIRMWARE_VERSION "v0.0"

#define DEFAULT_REMOTE_LIST_URL ""
#define DEFAULT_REMOTE_LIST false
#define DEFAULT_HAS_CHANNEL_POT false
#define DEFAULT_PCB_VERSION ""
#define DEFAULT_RADIO_ID ""

#define DEFAULT_STATION_ONE_URL "http://acc-radio.raiotech.com/mansfield.mp3"
#define DEFAULT_STATION_TWO_URL ""
#define DEFAULT_STATION_THREE_URL ""
#define DEFAULT_STATION_COUNT 1
#define DEFAULT_MAX_STATION_COUNT 4

#define LED_ON LOW
#define LED_OFF HIGH

#define PIN_CHANNEL_POT 2
#define PIN_LED_RED 4
#define PIN_LED_GREEN 5
#define PIN_LED_BLUE 6
#define PIN_VOLUME_POT 8
#define PIN_I2S_DOUT 12
#define PIN_I2S_BCLK 13
#define PIN_I2S_LRC 14

#define VOLUME_MIN 0
#define VOLUME_MAX 21

#define ANALOG_READ_INTERVAL_MS 50

#define DEBUG_MODE_TIMEOUT_MS 3000
#define DEBUG_STATUS_UPDATE_INTERVAL_MS 5000

WiFiManager wifiManager;
Audio audio;

bool debugMode = false;
unsigned long lastDebugStatusUpdate = 0;
uint32_t debugLPS = 0;

LEDStatus ledStatus(PIN_LED_RED, PIN_LED_GREEN, PIN_LED_BLUE, LED_OFF, LED_ON);

ChannelPot channelPot(PIN_CHANNEL_POT);

unsigned long lastAnalogRead = 0;
unsigned long lastMinuteInterval = 0;

Preferences preferences;

struct Config {
  String remoteListURL = DEFAULT_REMOTE_LIST_URL;
  bool remoteList = DEFAULT_REMOTE_LIST;
  String radioID = DEFAULT_RADIO_ID;
  bool hasChannelPot = DEFAULT_HAS_CHANNEL_POT;
  String pcbVersion = DEFAULT_PCB_VERSION;
  String stnOneURL = DEFAULT_STATION_ONE_URL;
  String stnTwoURL = DEFAULT_STATION_TWO_URL;
  String stnThreeURL = DEFAULT_STATION_THREE_URL;
  int stationCount = DEFAULT_STATION_COUNT;
  int maxStationCount = DEFAULT_MAX_STATION_COUNT;
};

Config config;

void getConfigFromPreferences() {
  // IMPORTANT the preferences library accepts keys up to 15 characters. Larger keys can be passed and no error will be thrown, but strange things may happen.
  preferences.begin("config", false);
  config.remoteListURL = preferences.getString("remoteListURL", DEFAULT_REMOTE_LIST_URL);
  config.remoteList = preferences.getBool("remoteList", DEFAULT_REMOTE_LIST);
  config.radioID = preferences.getString("radioID", DEFAULT_RADIO_ID);
  config.hasChannelPot = preferences.getBool("hasChannelPot", DEFAULT_HAS_CHANNEL_POT);
  config.pcbVersion = preferences.getString("pcbVersion", DEFAULT_PCB_VERSION);
  config.stnOneURL = preferences.getString("stnOneURL", DEFAULT_STATION_ONE_URL);
  config.stnTwoURL = preferences.getString("stnTwoURL", DEFAULT_STATION_TWO_URL);
  config.stnThreeURL = preferences.getString("stnThreeURL", DEFAULT_STATION_THREE_URL);
  config.stationCount = preferences.getInt("stationCount", DEFAULT_STATION_COUNT);
  config.maxStationCount = preferences.getInt("maxStationCount", DEFAULT_MAX_STATION_COUNT);
  preferences.end();
}

void putConfigToPreferences() {
  preferences.begin("config", false);
  preferences.putString("remoteListURL", config.remoteListURL);
  preferences.putBool("remoteList", config.remoteList);
  preferences.putString("radioID", config.radioID);
  preferences.putBool("hasChannelPot", config.hasChannelPot);
  preferences.putString("pcbVersion", config.pcbVersion);
  preferences.putString("stnOneURL", config.stnOneURL);
  preferences.putString("stnTwoURL", config.stnTwoURL);
  preferences.putString("stnThreeURL", config.stnThreeURL);
  preferences.putInt("stationCount", config.stationCount);
  preferences.putInt("maxStationCount", config.maxStationCount);
  preferences.end();
}

bool setDebugMode() {
  /*
  
  Use the radio object? Don't map, use a tolarance instead?
  
  */
  int volume = analogRead(PIN_VOLUME_POT);
  map(volume, 0, 4095, VOLUME_MIN, VOLUME_MAX);
  if (volume == VOLUME_MAX) {
    delay(DEBUG_MODE_TIMEOUT_MS);
    volume = analogRead(PIN_VOLUME_POT);
    map(volume, 0, 4095, VOLUME_MIN, VOLUME_MAX);
    if (volume == VOLUME_MIN) return true;
  }
  return false;
}

void debugHandleSerialInput() {
  while (Serial.available() > 0) {
    DynamicJsonDocument doc(12288);
    DeserializationError error = deserializeJson(doc, Serial);
    if (!error) {
      preferences.begin("config", false);

      serializeJson(doc, Serial);
      Serial.println("");

      config.remoteListURL = doc["remoteListURL"] | config.remoteListURL;
      config.remoteList = doc["remoteList"] | config.remoteList;
      config.radioID = doc["radioID"] | config.radioID;
      config.hasChannelPot = doc["hasChannelPot"];
      config.pcbVersion = doc["pcbVersion"] | config.pcbVersion;
      config.stnOneURL = doc["stnOneURL"] | config.stnOneURL;
      config.stnTwoURL = doc["stnTwoURL"] | config.stnTwoURL;
      config.stnThreeURL = doc["stnThreeURL"] | config.stnThreeURL;

      if (doc["stationCount"]) {
        config.stationCount = doc["stationCount"];
        channelPot.setNumberOfChannels(config.stationCount);
      }

      /* This is currently limited by the lookup table in the ChannelPot object */
      // config.maxStationCount = doc["maxStationCount"];

      putConfigToPreferences();

      if (doc["clearPreferences"]) {
        bool cleared = preferences.clear();
        Serial.println("Clearing preferences, restarting for this to take effect.");
        preferences.end();
        ESP.restart();
      }

      if (doc["resetWiFi"]) {
        wifiManager.resetSettings();
        Serial.println("Executing wifiManager.resetSettings(), restarting for this to take effect.");
        ESP.restart();
      }

      preferences.end();

      debugPrintConfigToSerial();
    }
  }
}

void debugPrintConfigToSerial() {
  Serial.printf("FIRMWARE_VERSION=%s\n", FIRMWARE_VERSION);
  Serial.printf("config.remoteListURL=%s\n", config.remoteListURL);
  Serial.printf("config.remoteList=%d\n", config.remoteList);
  Serial.printf("config.radioID=%s\n", config.radioID);
  Serial.printf("config.hasChannelPot=%d\n", config.hasChannelPot);
  Serial.printf("config.stnOneURL=%s\n", config.stnOneURL);
  Serial.printf("config.stnTwoURL=%s\n", config.stnTwoURL);
  Serial.printf("config.stnThreeURL=%s\n", config.stnThreeURL);
  Serial.printf("config.stationCount=%d\n", config.stationCount);
  Serial.printf("config.maxStationCount=%d\n", config.maxStationCount);
}

void configModeCallback(WiFiManager *myWiFiManager) {
  ledStatus.setStatus(LED_STATUS_WARNING_BLINKING);
  if (debugMode) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.print("Created config portal AP ");
    Serial.println(myWiFiManager->getConfigPortalSSID());
  }
}

void checkWiFiDisconnect() {
  // Check every minute if the wifi connection has been lost and reconnect.
  if (millis() > lastMinuteInterval + 60 * 1000) {
    if (debugMode) Serial.println("Checking WiFi connection...");
    if (WiFi.status() != WL_CONNECTED) {
      if (debugMode) Serial.println("Reconnecting to WiFi...");
      ledStatus.setStatus(LED_STATUS_WARNING_BLINKING);
      WiFi.disconnect();
      WiFi.reconnect();
    }
    // Reset interval
    lastMinuteInterval = millis();
  }
}

class Radio {
public:
  int mPlaying = false;
  int mChannelIdx = NULL;
  int mVolume = 0;
  char mCurrentStation[2048];
  Radio(){

  };
  void play() {
    String stations[] = {
      config.stnOneURL,
      config.stnTwoURL,
      config.stnThreeURL
    };
    ledStatus.setStatus(LED_STATUS_INFO_BLINKING);
    stations[mChannelIdx].toCharArray(mCurrentStation, 2048);
    mPlaying = audio.connecttohost(mCurrentStation);
    if (mPlaying) {
      ledStatus.setStatus(LED_STATUS_SUCCESS);
    } else {
      ledStatus.setStatus(LED_STATUS_WARNING_BLINKING);
    }
  };
  void stop() {
    mPlaying = false;
    audio.stopSong();
    ledStatus.setStatus(LED_STATUS_LIGHTS_OFF);
  };
  void setVolume() {
    int volumeRaw = analogRead(PIN_VOLUME_POT);
    mVolume = map(volumeRaw, 0, 4095, VOLUME_MIN, VOLUME_MAX);
    audio.setVolume(mVolume);
  }
  void setChannel() {
    int newChannelIdx = channelPot.selectedChannelIdx();
    if (mChannelIdx != newChannelIdx) {
      mChannelIdx = newChannelIdx;
      stop();
    }
  }
  void loop() {
    if (millis() > lastAnalogRead + ANALOG_READ_INTERVAL_MS) {
      setVolume();
      setChannel();
      lastAnalogRead = millis();
      if (mVolume > 0 && !mPlaying) {
        play();
      } else if (mVolume == 0 && mPlaying) {
        stop();
      }
    }
  }
};

Radio radio;

void debugModeLoop() {
  debugLPS++;
  debugHandleSerialInput();
  if (millis() - lastDebugStatusUpdate > DEBUG_STATUS_UPDATE_INTERVAL_MS) {
    Serial.printf("radio.mVolume=%d\n", radio.mVolume);
    Serial.printf("radio.mChannelIdx=%d\n", radio.mChannelIdx);
    Serial.printf("radio.mPlaying=%d\n", radio.mPlaying);
    Serial.print("radio.mCurrentStation=");
    Serial.println(radio.mCurrentStation);

    int lps = debugLPS / (DEBUG_STATUS_UPDATE_INTERVAL_MS / 1000);
    debugLPS = 0;
    Serial.printf("lps=%d\n", lps);

    lastDebugStatusUpdate = millis();
  }
}

void setup() {

  ledStatus.init();
  debugMode = setDebugMode();
  // debugMode = true;
  ledStatus.setStatus(LED_STATUS_FADE_SUCCESS_TO_INFO);
  ledStatus.setStatus(LED_STATUS_INFO);

  analogReadResolution(12);

  if (debugMode) {
    Serial.begin(115200);
    Serial.println("DEBUG MODE ON");
    ledStatus.setStatus(LED_STATUS_SUCCESS);
    delay(100);
    ledStatus.setStatus(LED_STATUS_INFO);
  }

  getConfigFromPreferences();
  if (debugMode) debugPrintConfigToSerial();

  channelPot.setNumberOfChannels(config.stationCount);

  // WiFiManager
  WiFi.mode(WIFI_STA);
  wifiManager.setDebugOutput(debugMode);
  wifiManager.setAPCallback(configModeCallback);
  bool res;
  res = wifiManager.autoConnect("Radio Setup");
  if (!res) {
    if (debugMode) Serial.println("Failed to connect or hit timeout");
    // This is a hard stop
    ledStatus.setStatus(LED_STATUS_ERROR_BLINKING);
    delay(5000);
    ESP.restart();
  }

  // Turn lights off if it connects to the remote list server, or does not need to.
  ledStatus.setStatus(LED_STATUS_LIGHTS_OFF);

  audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  audio.setVolume(0);
}

void loop() {
  // Check if the WiFi has disconnected
  checkWiFiDisconnect();

  radio.loop();
  audio.loop();

  if (debugMode) {
    debugModeLoop();
  }
}