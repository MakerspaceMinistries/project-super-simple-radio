/*

Example serial message for configuring the radio.
{ "remoteListURL": "remoteListURL", "remoteList": true, "radioID":"radioID", "hasChannelPot":true, "pcbVersion":"pcbVersion", "stnOneURL":"stnOneURL", "stnTwoURL":"stnTwoURL", "stnThreeURL":"stnThreeURL", "stationCount":2, "maxStationCount":3}

Example message for resetting the stored preferences and restarting the ESP.
{"clearPreferences": true}

*/

#include <ArduinoJson.h>
#include <Preferences.h>

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
#define DEFAULT_MAX_STATION_COUNT 1

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

#define DEBUG_MODE_TIMEOUT_MS 3000
#define DEBUG_STATUS_UPDATE_INTERVAL_MS 5000

bool debugMode = false;
unsigned long lastDebugStatusUpdate = 0;

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
  config.pcbVersion = preferences.getString("pcbVersion", DEFAULT_PCB_VERSION).c_str();
  config.stnOneURL = preferences.getString("stnOneURL", DEFAULT_STATION_ONE_URL).c_str();
  config.stnTwoURL = preferences.getString("stnTwoURL", DEFAULT_STATION_TWO_URL).c_str();
  config.stnThreeURL = preferences.getString("stnThreeURL", DEFAULT_STATION_THREE_URL).c_str();
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

int getVolume() {
  int volumeRaw = analogRead(PIN_VOLUME_POT);
  return map(volumeRaw, 0, 4095, VOLUME_MIN, VOLUME_MAX);
}

void setLightsRGB(int r, int g, int b) {
  digitalWrite(PIN_LED_RED, r);
  digitalWrite(PIN_LED_GREEN, g);
  digitalWrite(PIN_LED_BLUE, b);
}

bool setDebugMode() {
  if (getVolume() == VOLUME_MAX) {
    delay(DEBUG_MODE_TIMEOUT_MS);
    if (getVolume() == VOLUME_MIN) return true;
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
      config.stationCount = doc["stationCount"];
      config.maxStationCount = doc["maxStationCount"];

      putConfigToPreferences();

      if (doc["clearPreferences"]) {
        bool cleared = preferences.clear();
        Serial.println("Clearing preferences, restarting for this to take effect.");
        preferences.end();
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

void setup() {
  debugMode = setDebugMode();
  // debugMode = true;

  analogReadResolution(12);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);

  if (debugMode) {
    Serial.begin(115200);
    Serial.println("DEBUG MODE ON");
    setLightsRGB(LED_OFF, LED_OFF, LED_OFF);
    delay(100);
    setLightsRGB(LED_ON, LED_OFF, LED_OFF);
  }

  getConfigFromPreferences();
  if (debugMode) debugPrintConfigToSerial();


  setLightsRGB(LED_ON, LED_OFF, LED_OFF);
}

void loop() {

  int volume = getVolume();
  // audio.setVolume(volume);

  if (debugMode) {
    debugHandleSerialInput();
    if (millis() - lastDebugStatusUpdate > DEBUG_STATUS_UPDATE_INTERVAL_MS) {
      lastDebugStatusUpdate = millis();
      Serial.printf("volume=%d\n", volume);
    }
  }
}