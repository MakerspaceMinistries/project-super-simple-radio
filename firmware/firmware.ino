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

TODO:
  - Move the lights status to another file and its own class.
  - Add this as an option in debugHandleSerialInput(): wifiManager.resetSettings();
  - Make some sort of effect when the radio is first powered on, to be able to detect restarts later on.

*/

#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFiManager.h>

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

#define LED_STATUS_OFF -1
#define LED_STATUS_SUCCESS 0
#define LED_STATUS_SUCCESS_BLINKING 1
#define LED_STATUS_INFO 2
#define LED_STATUS_INFO_BLINKING 3
#define LED_STATUS_WARNING 4
#define LED_STATUS_WARNING_BLINKING 5
#define LED_STATUS_ERROR 6
#define LED_STATUS_ERROR_BLINKING 7

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

bool debugMode = false;
unsigned long lastDebugStatusUpdate = 0;
uint32_t debugLPS = 0;

hw_timer_t *timerBlink = NULL;
int rgbToBlink[] = { 0, 0, 0 };

unsigned long lastAnalogRead = 0;

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

int getVolume() {
  int volumeRaw = analogRead(PIN_VOLUME_POT);
  return map(volumeRaw, 0, 4095, VOLUME_MIN, VOLUME_MAX);
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

void configModeCallback(WiFiManager *myWiFiManager) {
  setLEDStatus(LED_STATUS_WARNING_BLINKING);
  if (debugMode) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.print("Created config portal AP ");
    Serial.println(myWiFiManager->getConfigPortalSSID());
  }
}

void blinkCallback() {
  // DO NOT change any variables here (that are from outside this scope)
  int pins[3] = { PIN_LED_RED, PIN_LED_GREEN, PIN_LED_BLUE };
  for (int i; i < 3; i++) {
    if (rgbToBlink[i] == 1) {
      int state = digitalRead(pins[i]);
      digitalWrite(pins[i], !state);
    }
  }
}

void setRGBToBlink(int r, int g, int b) {
  rgbToBlink[0] = r;
  rgbToBlink[1] = g;
  rgbToBlink[2] = b;
}

void blinkStart() {
  timerAlarmEnable(timerBlink);
}

void blinkTimerInit() {
  timerBlink = timerBegin(1, 80, true);
  timerAttachInterrupt(timerBlink, blinkCallback, true);
  timerAlarmWrite(timerBlink, 600000, true);
}

void blinkStop() {
  timerAlarmDisable(timerBlink);
  setRGBToBlink(0, 0, 0);
}

void setLEDStatus(int status) {
  setLightsRGB(LED_OFF, LED_OFF, LED_OFF);
  blinkStop();
  switch (status) {
    case LED_STATUS_OFF:
      // This is taken care of above.
      break;
    case LED_STATUS_SUCCESS:
      setLightsRGB(LED_OFF, LED_ON, LED_OFF);
      break;
    case LED_STATUS_SUCCESS_BLINKING:
      setLightsRGB(LED_OFF, LED_ON, LED_OFF);
      setRGBToBlink(0, 1, 0);
      blinkStart();
      break;
    case LED_STATUS_INFO:
      setLightsRGB(LED_OFF, LED_OFF, LED_ON);
      break;
    case LED_STATUS_INFO_BLINKING:
      setLightsRGB(LED_OFF, LED_OFF, LED_ON);
      setRGBToBlink(0, 0, 1);
      blinkStart();
      break;
    case LED_STATUS_WARNING:
      setLightsRGB(LED_ON, LED_ON, LED_OFF);
      break;
    case LED_STATUS_WARNING_BLINKING:
      setLightsRGB(LED_ON, LED_ON, LED_OFF);
      setRGBToBlink(1, 1, 0);
      blinkStart();
      break;
    case LED_STATUS_ERROR:
      setLightsRGB(LED_ON, LED_OFF, LED_OFF);
      break;
    case LED_STATUS_ERROR_BLINKING:
      setLightsRGB(LED_ON, LED_OFF, LED_OFF);
      setRGBToBlink(1, 0, 0);
      blinkStart();
      break;
  }
}

void setLightsRGB(int r, int g, int b) {
  digitalWrite(PIN_LED_RED, r);
  digitalWrite(PIN_LED_GREEN, g);
  digitalWrite(PIN_LED_BLUE, b);
}

void setup() {

  // setup lights
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);

  blinkTimerInit();
  setLEDStatus(LED_STATUS_INFO);

  debugMode = setDebugMode();
  // debugMode = true;

  analogReadResolution(12);

  if (debugMode) {
    Serial.begin(115200);
    Serial.println("DEBUG MODE ON");
    setLEDStatus(LED_STATUS_SUCCESS);
    delay(100);
    setLEDStatus(LED_STATUS_INFO);
  }

  getConfigFromPreferences();
  if (debugMode) debugPrintConfigToSerial();

  // WiFiManager
  WiFi.mode(WIFI_STA);
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  bool res;
  res = wifiManager.autoConnect("Radio Setup");
  if (!res) {
    if (debugMode) Serial.println("Failed to connect or hit timeout");
    // This is a hard stop
    setLEDStatus(LED_STATUS_ERROR_BLINKING);
    delay(5000);
    ESP.restart();
  }

  // Turn lights off if it connects to the remote list server, or does not need to.
  setLEDStatus(LED_STATUS_OFF);
}

void loop() {

  int volume;
  if (millis() > lastAnalogRead + ANALOG_READ_INTERVAL_MS) {
    volume = getVolume();
    // audio.setVolume(volume);
    lastAnalogRead = millis();
  }

  if (debugMode) {
    debugLPS++;
    debugHandleSerialInput();
    if (millis() - lastDebugStatusUpdate > DEBUG_STATUS_UPDATE_INTERVAL_MS) {
      Serial.printf("volume=%d\n", volume);

      int lps = debugLPS / (DEBUG_STATUS_UPDATE_INTERVAL_MS / 1000);
      debugLPS = 0;
      Serial.printf("lps=%d\n", lps);

      lastDebugStatusUpdate = millis();
    }
  }
}