/*

Radio contains the current state of the radio hardware, as well as some settings.

It is unaware of the Audio & LEDStatus objects.

*/
#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Audio.h"
#include "LEDStatus.h"

// Move these to the radio config?
// #define RADIO_CURRENT_TIME_CHECK_INTERVAL_S 3

WiFiClient client;
HTTPClient http;

struct RadioConfig {

  // Hardware
  int pinChannelPot = 2;
  int pinVolumePot = 8;
  int pinDacSdMode = 11;
  int pinI2sDout = 12;
  int pinI2sBclk = 13;
  int pinI2sLrc = 14;
  bool hasChannelPot = true;
  String pcbVersion = "";
  int analogReadResolution = 12;  // This ensures the maps used in readChannelIdx and readVolume are correct.

  // Software
  int volumeMin = 0;
  int volumeMax = 21;
  String stn1URL = "";
  String stn2URL = "";
  String stn3URL = "";
  String stn4URL = "";
  int stationCount = 1;
  int maxStationCount = 4;

  // Intervals
  int debugStatusUpdateIntervalMs = 5000;
  int analogReadIntervalMs = 50;
  int streamLossDetectionIntervalS = 3;

  // Remote Config
  bool remoteConfig = false;
  String remoteConfigURL = "";
  String radioID = "";
};

struct RadioStatus {

  // This will be replaced by the below
  bool playing;

  // These statuses are parsed to set the status code, which is used to set the LEDs

  // Status code, used to set the LEDs
  int statusCode;

  // Raw inputs
  int channelIdxInput = 0;
  int volumeInput = 0;

  // Settings
  int channelIdx = 0;
  int volume = 0;

  // Statuses (Associated with actions)
  bool wifiConnected = false;       // Satus of WiFi after booting. (The initial connection is handled by WifiManager, and the LEDs should be set there.)
  bool streamConnecting = false;    // This is set before connecting to a new stream (whether starting or when changing stations)
  bool streamReConnecting = false;  // Connection after a lost connection
  bool streamPlaying = false;       // (this is volume > 0 and connection successful) Changing volume from 0 to something greater than 0 requests the stream. Changing the volume to 0 stops the stream.
  bool streamIsAdvancing = false;   // This is set by checking if the number of seconds played has advanced

  // These are set during blocking and should be reported as errors, not statuses - ex: initialConnectionSuccessful is by default false until it's set to true - but it's not an error until the connection times out
  // bool streamInitialConnectionSuccessful = false;  // Starting the stream returns success/failure, which goes here.

  /*

Set error code vs set status code??? (or set status code which is an error cude, set_status(100, error=true))
Clear error code????

*/

  /*
  status code numbers.

  000 Idle/playing/buffering  (Success/Info)
  100 connection errors       (Warning)
  200 network errors          (Error)

  Next, make a list of possible statuses and give them numbers and put them into the above LED Status colors

  #define RADIO_STATUS_CODE_IDLE        0
  #define RADIO_STATUS_CODE_PLAYING     1

  add interpreted_status to the status struct, and compare to current status, if it's different, update.

  Do not overwrite a code with one that is smaller!
*/
};

class Radio {
  unsigned long lastVolumeChannelRead = 0;
  unsigned long lastCurrentTimeCheck = 0;
  uint32_t lastCurrentTime = 0;

  /*

    Split the fader up into 13 parts (0-12), which correspond with 3 channels.

    0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12
    |    3    |              2            |      1      |
    The channels at the sides of potentiometer need less room since they locate easily by butting up against the end of the fader.

  */
  int mChannelsLookupTable[4][12] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2 },
    { 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3 }
  };

  void handleDebugMode();
  void debugHandleSerialInput();
  void checkWiFiDisconnect();

  // Debugging. If debugMode is false, these are unused.
  unsigned long lastDebugStatusUpdate = 0;
  uint32_t debugLPS = 0;

  // TODO lastMinuteInterval should have a better name, or the function that uses it should.
  unsigned long lastMinuteInterval = 0;

public:
  Radio(RadioConfig *config, WiFiManager *myWifiManager, Audio *myAudio);
  void init();
  void getConfigFromPreferences();
  void putConfigToPreferences();
  int readChannelIdx();
  int readVolume();
  void initDebugMode();
  bool getConfigFromRemote();
  void debugPrintConfigToSerial();
  void setStatusCode();
  void loop();
  void debugModeLoop();
  void setDacSdMode(bool enable);
  bool debugMode = false;
  Preferences preferences;
  LEDStatus ledStatus;
  RadioStatus status;
  RadioConfig *config;
  WiFiManager *wifiManager;
  Audio *audio;
};

Radio::Radio(RadioConfig *myConfig, WiFiManager *myWifiManager, Audio *myAudio) {
  config = myConfig;
  wifiManager = myWifiManager;
  audio = myAudio;

  status.playing = false;
  status.channelIdx = 0;
}

void Radio::getConfigFromPreferences() {
  // IMPORTANT the preferences library accepts keys up to 15 characters. Larger keys can be passed and no error will be thrown, but strange things may happen.
  preferences.begin("config", false);
  config->remoteConfigURL = preferences.getString("remoteConfigURL", config->remoteConfigURL);
  config->remoteConfig = preferences.getBool("remoteConfig", config->remoteConfig);
  config->radioID = preferences.getString("radioID", config->radioID);
  config->hasChannelPot = preferences.getBool("hasChannelPot", config->hasChannelPot);
  config->pcbVersion = preferences.getString("pcbVersion", config->pcbVersion);
  config->stn1URL = preferences.getString("stn1URL", config->stn1URL);
  config->stn2URL = preferences.getString("stn2URL", config->stn2URL);
  config->stn3URL = preferences.getString("stn3URL", config->stn3URL);
  config->stn4URL = preferences.getString("stn4URL", config->stn4URL);
  config->stationCount = preferences.getInt("stationCount", config->stationCount);
  preferences.end();
}

void Radio::putConfigToPreferences() {
  preferences.begin("config", false);
  preferences.putString("remoteConfigURL", config->remoteConfigURL);
  preferences.putBool("remoteConfig", config->remoteConfig);
  preferences.putString("radioID", config->radioID);
  preferences.putBool("hasChannelPot", config->hasChannelPot);
  preferences.putString("pcbVersion", config->pcbVersion);
  preferences.putString("stn1URL", config->stn1URL);
  preferences.putString("stn2URL", config->stn2URL);
  preferences.putString("stn3URL", config->stn3URL);
  preferences.putString("stn4URL", config->stn4URL);
  preferences.putInt("stationCount", config->stationCount);
  preferences.end();
}

int Radio::readChannelIdx() {
  int channelInput = analogRead(config->pinChannelPot);
  channelInput = map(channelInput, 0, 4095, 0, 11);
  return mChannelsLookupTable[config->stationCount - 1][channelInput];
}

int Radio::readVolume() {
  int volumeRaw = analogRead(config->pinVolumePot);
  int volume = map(volumeRaw, 0, 4095, config->volumeMin, config->volumeMax);
  return volume;
}

void Radio::setDacSdMode(bool enable) {
  // Sets the SD mode pin that's connected to the DAC. true turns the DAC on, false turns the dac off.
  if (enable) {
    digitalWrite(config->pinDacSdMode, HIGH);
  } else {
    digitalWrite(config->pinDacSdMode, LOW);
  }
}

void Radio::init() {

  analogReadResolution(config->analogReadResolution);

  pinMode(config->pinDacSdMode, OUTPUT);

  getConfigFromPreferences();
  initDebugMode();

  ledStatus.init();

  if (debugMode) {
    Serial.begin(115200);
    Serial.println("DEBUG MODE ON");
    ledStatus.setStatus(LED_STATUS_SUCCESS);
    delay(100);
    ledStatus.setStatus(LED_STATUS_INFO);
    debugPrintConfigToSerial();
  }

  ledStatus.setStatus(LED_STATUS_FADE_SUCCESS_TO_INFO);
  ledStatus.setStatus(LED_STATUS_INFO);

  // Initialize WiFi/WiFiManager
  WiFi.mode(WIFI_STA);
  wifiManager->setDebugOutput(debugMode);
  bool res;
  res = wifiManager->autoConnect("Radio Setup");
  if (!res) {
    if (debugMode) Serial.println("Failed to connect or hit timeout");
    // This is a hard stop
    ledStatus.setStatus(LED_STATUS_ERROR_BLINKING);
    delay(5000);
    ESP.restart();
  }

  // Initialize Audio
  audio->setPinout(config->pinI2sBclk, config->pinI2sLrc, config->pinI2sDout);
  audio->setVolume(0);

  // Get config from remote server

  ledStatus.setStatus(LED_STATUS_LIGHTS_OFF);

  // Get stations from config server
  bool error = getConfigFromRemote();
  if (error) {
    ledStatus.setStatus(LED_STATUS_WARNING_BLINKING);
    delay(5000);
  }

};

void Radio::initDebugMode() {
  int volume = readVolume();
  if (volume == config->volumeMax) {
    delay(3000);
    volume = readVolume();
    if (volume == config->volumeMin) {
      debugMode = true;
      return;
    };
  }
  debugMode = false;
}

bool Radio::getConfigFromRemote() {
  // returns error: true|false
  if (!config->remoteConfig) {
    return false;
  }

  char hasPot[5];
  (config->hasChannelPot) ? strcpy(hasPot, "true") : strcpy(hasPot, "false");
  String url = config->remoteConfigURL
               + config->radioID
               + "?pcb_version=" + config->pcbVersion
               + "&firmware_version=" + FIRMWARE_VERSION
               + "&max_station_count=" + (String)config->maxStationCount
               + "&has_channel_potentiometer=" + hasPot;

  if (debugMode) {
    Serial.print(F("Remote config: url="));
    Serial.println(url);
  }

  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.useHTTP10(true);
  http.begin(client, url);
  http.GET();

  Serial.begin(115200);

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, http.getStream());
  if (error) {
    if (debugMode) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
    }
    http.end();
    return true;
  }

  config->stn1URL = doc["stn1URL"].as<String>();
  config->stn2URL = doc["stn2URL"].as<String>();
  config->stn3URL = doc["stn3URL"].as<String>();
  config->stn4URL = doc["stn4URL"].as<String>();
  config->stationCount = doc["stationCount"];
  putConfigToPreferences();

  if (debugMode) {
    Serial.println("Sucessfully retrieved config from remote server.");
  }

  // If the stations are passed as an array, access the array like this:
  // JsonArray station_urls = doc["station_urls"].as<JsonArray>();

  http.end();

  return false;
}

void Radio::debugPrintConfigToSerial() {
  Serial.printf("FIRMWARE_VERSION=%s\n", FIRMWARE_VERSION);
  Serial.println("-----------");
  Serial.println("Radio Config");
  Serial.print("remoteConfigURL=");
  Serial.println(config->remoteConfigURL);
  Serial.printf("remoteConfig=%d\n", config->remoteConfig);
  Serial.printf("radioID=%s\n", config->radioID);
  Serial.printf("hasChannelPot=%d\n", config->hasChannelPot);
  Serial.print("stn1URL=");
  Serial.println(config->stn1URL);
  Serial.print("stn2URL=");
  Serial.println(config->stn2URL);
  Serial.print("stn3URL=");
  Serial.println(config->stn3URL);
  Serial.print("stn4URL=");
  Serial.println(config->stn4URL);
  Serial.printf("stationCount=%d\n", config->stationCount);
  Serial.printf("maxStationCount=%d\n", config->maxStationCount);
}

void Radio::debugModeLoop() {
  debugLPS++;
  debugHandleSerialInput();
  if (millis() - lastDebugStatusUpdate > config->debugStatusUpdateIntervalMs) {
    Serial.printf("radio.readVolume()=%d\n", readVolume());
    Serial.print("radio.readChannelIdx()=");
    Serial.println(readChannelIdx());
    Serial.printf("radio.status.channelIdx=%d\n", status.channelIdx);
    Serial.printf("radio.status.playing=%d\n", status.playing);

    int lps = debugLPS / (config->debugStatusUpdateIntervalMs / 1000);
    debugLPS = 0;
    Serial.printf("lps=%d\n", lps);

    lastDebugStatusUpdate = millis();
  }
}

void Radio::debugHandleSerialInput() {
  while (Serial.available() > 0) {
    DynamicJsonDocument doc(12288);
    DeserializationError error = deserializeJson(doc, Serial);
    if (!error) {

      serializeJson(doc, Serial);
      Serial.println("");

      config->remoteConfigURL = doc["remoteConfigURL"] | config->remoteConfigURL;
      config->remoteConfig = doc["remoteConfig"] | config->remoteConfig;
      config->radioID = doc["radioID"] | config->radioID;

      // should the stuff after | be there? if it works, then leave it alone
      config->hasChannelPot = doc["hasChannelPot"] | config->hasChannelPot;
      config->pcbVersion = doc["pcbVersion"] | config->pcbVersion;
      config->stn1URL = doc["stn1URL"] | config->stn1URL;
      config->stn2URL = doc["stn2URL"] | config->stn2URL;
      config->stn3URL = doc["stn3URL"] | config->stn3URL;
      config->stn4URL = doc["stn4URL"] | config->stn4URL;

      if (doc["stationCount"]) {
        config->stationCount = doc["stationCount"];
      }

      putConfigToPreferences();

      if (doc["clearPreferences"]) {
        preferences.begin("config", false);
        bool cleared = preferences.clear();
        Serial.println("Clearing preferences, restarting for this to take effect.");
        preferences.end();
        ESP.restart();
      }

      if (doc["resetWiFi"]) {
        wifiManager->resetSettings();
        Serial.println("Executing wifiManager.resetSettings(), restarting for this to take effect.");
        ESP.restart();
      }

      debugPrintConfigToSerial();
    }
  }
}

void Radio::checkWiFiDisconnect() {
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

void Radio::loop() {

  audio->loop();

  if (debugMode) {
    debugModeLoop();
  }

  // Check if the WiFi has disconnected
  checkWiFiDisconnect();

  if (millis() > lastVolumeChannelRead + config->analogReadIntervalMs) {

    // Read the current state of the radio's inputs
    int selectedVolume = readVolume();
    int selectedChannelIdx = readChannelIdx();

    // Setting volume doesn't use extra resources, do it even if there were no changes
    audio->setVolume(selectedVolume);

    /*

    Check if the requested status is different from the current status, if different, make changes
    
    */

    if (selectedChannelIdx != status.channelIdx) {
      // change channel, set status.playing to false to trigger a reconnect.
      status.channelIdx = selectedChannelIdx;
      status.playing = false;
    }

    if (selectedVolume > 0 && !status.playing) {

      setDacSdMode(true);  // Turn DAC on

      // start play, set status.playing
      ledStatus.setStatus(LED_STATUS_INFO_BLINKING);

      String* channels[] = { &config->stn1URL, &config->stn2URL, &config->stn3URL, &config->stn4URL };
      char selectedChannelURL[2048];
      channels[status.channelIdx]->toCharArray(selectedChannelURL, 2048);

      status.playing = audio->connecttohost(selectedChannelURL);

      // This may overwrite an error status since it doesn't rely on a status change, but fires every VOLUME_CHANNEL_READ_INTERVAL_MS
      if (status.playing) {
        ledStatus.setStatus(LED_STATUS_SUCCESS);
      } else {
        ledStatus.setStatus(LED_STATUS_WARNING_BLINKING);
      }

      if (debugMode) {
        Serial.print("selectedChannelURL=");
        Serial.println(selectedChannelURL);
      }
    }

    if (selectedVolume == 0 && status.playing) {
      // stop play, set status
      setDacSdMode(false);  // Turn DAC off
      status.playing = false;
      audio->stopSong();
      ledStatus.setStatus(LED_STATUS_LIGHTS_OFF);
    }

    // Check if the audio->getAudioCurrentTime() is advancing, if not, set the status to match (triggering a reconnect).
    if (status.playing && millis() > lastCurrentTimeCheck + config->streamLossDetectionIntervalS * 1000) {
      lastCurrentTimeCheck = millis();
      uint32_t currentTime = audio->getAudioCurrentTime();

      // Give the stream config->streamLossDetectionIntervalS * 2 to get started (otherwise this may check too soon and start a loop of reconnects)
      if (currentTime > config->streamLossDetectionIntervalS * 2 && currentTime - lastCurrentTime < config->streamLossDetectionIntervalS * 0.9) {
        status.playing = false;
        lastCurrentTime = currentTime;
        if (debugMode) {
          Serial.print("Connection drop detected, triggering a reconnect.");
        }
      }
    }
  }
}
