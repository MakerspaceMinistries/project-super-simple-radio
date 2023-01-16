/*

Example serial message for configuring the radio.
{ "remoteListURL": "remoteListURL", "remoteList": true, "rID":"rID", "hasChannelPot":true, "pcbVersion":"pcbVersion", "stnOneURL":"stnOneURL", "stnTwoURL":"stnTwoURL", "stnThreeURL":"stnThreeURL", "stationCount":2, "maxStationCount":3}

Example message for resetting the stored preferences and restarting the ESP.
{"clearPreferences": true}

*/

#include <ArduinoJson.h>
#include <Preferences.h>

#define FIRMWARE_VERSION "v0.0"

#define DEFAULT_REMOTE_STATION_LIST_URL ""
#define DEFAULT_REMOTE_STATION_LIST_ENABLED false
#define DEFAULT_HAS_CHANNEL_POT false
#define DEFAULT_PCB_VERSION ""
#define DEFAULT_R_ID ""

#define DEFAULT_STATION_ONE_URL ""
#define DEFAULT_STATION_TWO_URL ""
#define DEFAULT_STATION_THREE_URL ""
#define DEFAULT_STATION_COUNT 1
#define DEFAULT_MAX_STATION_COUNT 1

#define LED_ON LOW
#define LED_OFF HIGH
#define LED_PIN_RED 4
#define LED_PIN_GREEN 5
#define LED_PIN_BLUE 6
#define VOLUME_POT_PIN 8
#define VOLUME_MIN 0
#define VOLUME_MAX 20
#define CHANNEL_POT_PIN 2
#define DEBUG_MODE_TIMEOUT_MS 3000

#define I2S_DOUT 12
#define I2S_BCLK 13
#define I2S_LRC 14

int volume = 0;
bool debugMode = false;

Preferences preferences;

// Name Struct with the config
struct Config {
  String remoteListURL = DEFAULT_REMOTE_STATION_LIST_URL;
  bool remoteList = DEFAULT_REMOTE_STATION_LIST_ENABLED;
  String rID = DEFAULT_R_ID;
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
  config.remoteListURL = preferences.getString("remoteListURL", DEFAULT_REMOTE_STATION_LIST_URL);
  config.remoteList = preferences.getBool("remoteList", DEFAULT_REMOTE_STATION_LIST_ENABLED);
  config.rID = preferences.getString("rID", DEFAULT_R_ID);
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
  preferences.putString("rID", config.rID);
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
  int volumeRaw = analogRead(VOLUME_POT_PIN);
  return map(volumeRaw, 0, 4095, VOLUME_MIN, VOLUME_MAX);
}

void setLightsRGB(int r, int g, int b) {
  digitalWrite(LED_PIN_RED, r);
  digitalWrite(LED_PIN_GREEN, g);
  digitalWrite(LED_PIN_BLUE, b);
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
      config.rID = doc["rID"] | config.rID;
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
  Serial.printf("config.rID=%s\n", config.rID);
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

  if (debugMode) {
    Serial.begin(115200);
    Serial.println("Debug Mode On");
    setLightsRGB(LED_OFF, LED_OFF, LED_OFF);
    delay(100);
    setLightsRGB(LED_ON, LED_OFF, LED_OFF);
  }

  getConfigFromPreferences();
  if (debugMode) debugPrintConfigToSerial();

  analogReadResolution(12);

  pinMode(LED_PIN_RED, OUTPUT);
  pinMode(LED_PIN_GREEN, OUTPUT);
  pinMode(LED_PIN_BLUE, OUTPUT);

  setLightsRGB(LED_ON, LED_OFF, LED_OFF);
}

void loop() {

  int newVolume = getVolume();
  if (volume != newVolume) {
    volume = newVolume;
    // audio.setVolume(volume);
    if (debugMode) Serial.printf("Volume set to: %d\n", volume);
  }

  if (debugMode) {
    debugHandleSerialInput();
  }
}