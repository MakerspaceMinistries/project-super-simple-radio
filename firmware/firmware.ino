#include <ArduinoJson.h>
#include <Preferences.h>

#define FIRMWARE_VERSION "v0.0"

#define DEFAULT_STATION_LIST_URL ""
#define URL_SIZE 2048
#define DEFAULT_STATION_LIST_ENABLED false
#define DEFAULT_HAS_CHANNEL_POT false
#define DEFAULT_PCB_VERSION ""
#define PCB_VERSION_SIZE 32
#define DEFAULT_R_ID ""
#define R_ID_SIZE 64

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
  char remoteStationListURL[URL_SIZE] = DEFAULT_STATION_LIST_URL;
  bool remoteStationListEnabled = DEFAULT_STATION_LIST_ENABLED;
  char rID[R_ID_SIZE] = DEFAULT_R_ID;
  bool hasChannelPot = DEFAULT_HAS_CHANNEL_POT;
  char pcbVersion[PCB_VERSION_SIZE] = DEFAULT_PCB_VERSION;
  char stationOneURL[URL_SIZE] = DEFAULT_STATION_ONE_URL;
  char stationTwoURL[URL_SIZE] = DEFAULT_STATION_TWO_URL;
  char stationThreeURL[URL_SIZE] = DEFAULT_STATION_THREE_URL;
  int stationCount = DEFAULT_STATION_COUNT;
  int maxStationCount = DEFAULT_MAX_STATION_COUNT;
};

Config config;

void getConfigFromPreferences() {
  preferences.getBytes("remoteStationListURL", config.remoteStationListURL, URL_SIZE);
  config.remoteStationListEnabled = preferences.getBool("remoteStationListEnabled", DEFAULT_STATION_LIST_ENABLED);
  preferences.getBytes("rID", config.rID, R_ID_SIZE);
  config.hasChannelPot = preferences.getBool("hasChannelPot", DEFAULT_HAS_CHANNEL_POT);
  preferences.getBytes("pcbVersion", config.pcbVersion, PCB_VERSION_SIZE);
  preferences.getBytes("stationOneURL", config.stationOneURL, URL_SIZE);
  preferences.getBytes("stationTwoURL", config.stationTwoURL, URL_SIZE);
  preferences.getBytes("stationThreeURL", config.stationThreeURL, URL_SIZE);
  config.stationCount = preferences.getInt("stationCount", DEFAULT_STATION_COUNT);
  config.maxStationCount = preferences.getInt("maxStationCount", DEFAULT_MAX_STATION_COUNT);
}

void putConfigToPreferences() {
  preferences.putBytes("remoteStationListURL", config.remoteStationListURL, URL_SIZE);
  preferences.putBool("remoteStationListEnabled", config.remoteStationListEnabled);
  preferences.putBytes("rID", config.rID, R_ID_SIZE);
  preferences.putBool("hasChannelPot", config.hasChannelPot);
  preferences.putBytes("pcbVersion", config.pcbVersion, PCB_VERSION_SIZE);
  preferences.putBytes("stationOneURL", config.stationOneURL, URL_SIZE);
  preferences.putBytes("stationTwoURL", config.stationTwoURL, URL_SIZE);
  preferences.putBytes("stationThreeURL", config.stationThreeURL, URL_SIZE);
  preferences.putInt("stationCount", config.stationCount);
  preferences.putInt("maxStationCount", config.maxStationCount);
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

      strlcpy(config.remoteStationListURL, doc["remoteStationListURL"] | config.remoteStationListURL, sizeof(config.remoteStationListURL));
      config.remoteStationListEnabled = doc["remoteStationListEnabled"] | config.remoteStationListEnabled;
      strlcpy(config.rID, doc["rID"] | config.rID, sizeof(config.rID));
      config.hasChannelPot = doc["hasChannelPot"];
      strlcpy(config.pcbVersion, doc["pcbVersion"] | config.pcbVersion, sizeof(config.pcbVersion));
      strlcpy(config.stationOneURL, doc["stationOneURL"] | config.stationOneURL, sizeof(config.stationOneURL));
      strlcpy(config.stationTwoURL, doc["stationTwoURL"] | config.stationTwoURL, sizeof(config.stationTwoURL));
      strlcpy(config.stationThreeURL, doc["stationThreeURL"] | config.stationThreeURL, sizeof(config.stationThreeURL));
      config.stationCount = doc["stationCount"];
      config.maxStationCount = doc["maxStationCount"];

      putConfigToPreferences();

      if (doc["clearPreferences"]) {
        bool cleared = preferences.clear();
        Serial.println("Clearing preferences, be sure to restart for this to take effect.");
      }

      preferences.end();

      debugPrintConfigToSerial();
    }
  }
}

void debugPrintConfigToSerial() {
  Serial.printf("FIRMWARE_VERSION=%s\n", FIRMWARE_VERSION);
  Serial.printf("config.remoteStationListURL=%s\n", config.remoteStationListURL);
  Serial.printf("config.remoteStationListEnabled=%d\n", config.remoteStationListEnabled);
  Serial.printf("config.rID=%s\n", config.rID);
  Serial.printf("config.hasChannelPot=%d\n", config.hasChannelPot);
  Serial.printf("config.stationOneURL=%s\n", config.stationOneURL);
  Serial.printf("config.stationTwoURL=%s\n", config.stationTwoURL);
  Serial.printf("config.stationThreeURL=%s\n", config.stationThreeURL);
  Serial.printf("config.stationCount=%d\n", config.stationCount);
  Serial.printf("config.maxStationCount=%d\n", config.maxStationCount);
}

void setup() {
  preferences.begin("config", false);

  analogReadResolution(12);

  pinMode(LED_PIN_RED, OUTPUT);
  pinMode(LED_PIN_GREEN, OUTPUT);
  pinMode(LED_PIN_BLUE, OUTPUT);

  setLightsRGB(LED_ON, LED_OFF, LED_OFF);

  debugMode = setDebugMode();
  // debugMode = true;

  if (debugMode) {
    Serial.begin(115200);
    Serial.println("Debug Mode On");
    setLightsRGB(LED_OFF, LED_OFF, LED_OFF);
    delay(100);
    setLightsRGB(LED_ON, LED_OFF, LED_OFF);
    debugPrintConfigToSerial();
  }
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