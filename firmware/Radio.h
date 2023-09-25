/*



*/
#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Audio.h"
#include "LEDStatus.h"

/* ERRORS */
// Setup
#define RADIO_STATUS_300_UNABLE_TO_CONNECT_TO_WIFI_WM_ACTIVE 300
#define RADIO_STATUS_350_FAILED_TO_CONNECT_AFTER_WIFI_MANAGER 350
// Loop
#define RADIO_STATUS_301_WIFI_CONNECTION_LOST 301

/* WARNINGS */
// Setup
#define RADIO_STATUS_250_UNABLE_TO_CONNECT_TO_CONFIG_SERVER 250
// Loop
#define RADIO_STATUS_251_STREAM_CONNECTION_LOST_RECONNECTING 251
#define RADIO_STATUS_252_UNABLE_TO_CONNECT_WAITING_AND_TRYING_AGAIN 252

/* SUCCESS */
// Loop
#define RADIO_STATUS_101_PLAYING 101

/* INFO */
// Setup
#define RADIO_STATUS_001_RADIO_INITIALIZING 1
// Loop
#define RADIO_STATUS_051_INITIAL_STREAMING_CONNECTION 51

// Special Idle Status, which turns LEDs off
#define RADIO_STATUS_000_IDLE 0

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
  int statusCheckIntervalMs = 50;
  int streamLossDetectionWindowS = 5;  // Less than 5 may cause rounding issues

  // Remote Config
  bool remoteConfig = false;
  String remoteConfigURL = "";
  String radioID = "";
};

struct RadioStatus {

  // This will be replaced by the below

  // Inputs
  int channelIdxInput = 0;
  int volumeInput = 0;
  bool playInput = false;

  // Outputs
  int channelIdxOutput = 0;
  bool playOutput = false;

  bool reconnecting = false;

  // Statuses (Associated with actions)
  bool playing = false;          // (this is volume > 0 and connection successful) Changing volume from 0 to something greater than 0 requests the stream. Changing the volume to 0 stops the stream.
  bool streamAdvancing = false;  // This is set by checking if the number of seconds played has advanced
};

class Radio {
  unsigned long lastStatusCheck = 0;
  uint32_t lastCurrentTime = 0;

  /*

    Example: Split the fader up into 13 parts (0-12), which correspond with 3 channels.

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
  void handleSerialInput();

  // Debugging. If debugMode is false, these are unused.
  unsigned long lastDebugStatusUpdate = 0;
  uint32_t debugLPS = 0;  // Loops per second

public:
  Radio(RadioConfig *config, WiFiManager *myWifiManager, Audio *myAudio);
  void init();
  void getConfigFromPreferences();
  void putConfigToPreferences();
  int readChannelIdx();
  int readVolume();
  void initDebugMode();
  bool getConfigFromRemote();
  void printConfigToSerial();
  void setStatusCode();
  void loop();
  void debugModeLoop();
  void setDacSdMode(bool enable);
  bool connectToStream();
  bool streamIsAdvancing();
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
}

bool Radio::connectToStream() {

  setDacSdMode(true);  // Turn DAC on

  String *channels[] = { &config->stn1URL, &config->stn2URL, &config->stn3URL, &config->stn4URL };
  char selectedChannelURL[2048];
  channels[status.channelIdxOutput]->toCharArray(selectedChannelURL, 2048);

  if (debugMode) {
    Serial.print("selectedChannelURL=");
    Serial.println(selectedChannelURL);
  }

  return audio->connecttohost(selectedChannelURL);
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

  Serial.begin(115200);

  initDebugMode();
  ledStatus.init(debugMode);

  getConfigFromPreferences();

  if (debugMode) {
    Serial.println("DEBUG MODE ON");
    ledStatus.setStatus(LED_STATUS_SUCCESS);
    delay(100);
    ledStatus.setStatus(LED_STATUS_INFO);
    printConfigToSerial();
  }

  ledStatus.setStatus(LED_STATUS_FADE_SUCCESS_TO_INFO);
  ledStatus.setStatusCode(RADIO_STATUS_001_RADIO_INITIALIZING);

  analogReadResolution(config->analogReadResolution);

  pinMode(config->pinDacSdMode, OUTPUT);

  // Initialize WiFi/WiFiManager
  WiFi.mode(WIFI_STA);

  // Example for setting a MAC address - useful for networks which require a registration and then filter by MAC address. Use a laptop to log in to the network (adding the laptop's MAC to the whitelist) and then set the radio's MAC address to match the laptop's address.
  // This needs to come after WiFi.mode(WIFI_STA);
  // uint8_t macAddy[] = { 0x08, 0x71, 0x90, 0x89, 0x85, 0x87 };
  // esp_wifi_set_mac(WIFI_IF_STA, macAddy);

  wifiManager->setDebugOutput(debugMode);
  bool res;
  res = wifiManager->autoConnect("Radio Setup");
  if (!res) {
    if (debugMode) Serial.println("Failed to connect or hit timeout");
    // This is a hard stop
    ledStatus.setStatusCode(RADIO_STATUS_350_FAILED_TO_CONNECT_AFTER_WIFI_MANAGER, 10000);
    ESP.restart();
  }

  // Initialize Audio
  audio->setPinout(config->pinI2sBclk, config->pinI2sLrc, config->pinI2sDout);
  audio->setVolume(0);

  // Get config from remote server
  bool error = getConfigFromRemote();
  if (error) {
    // Show the error, but move on since the radio should be able to use the config stored in preferences.
    ledStatus.setStatusCode(RADIO_STATUS_250_UNABLE_TO_CONNECT_TO_CONFIG_SERVER, 10000);
    ledStatus.clearStatusCode(RADIO_STATUS_250_UNABLE_TO_CONNECT_TO_CONFIG_SERVER);
  }

  ledStatus.setStatusCode(RADIO_STATUS_000_IDLE);
};

void Radio::initDebugMode() {

  // If debug mode was set to true elsewhere (as part of the setup()), don't check the volume.
  if (debugMode == true) return;

  // Detect if debug mode is being requested by having the volume set to full when turned on (or reset) and then turned to 0 within 3 seconds.
  int volume = readVolume();
  if (volume > config->volumeMax - 2) {
    delay(3000);
    volume = readVolume();
    if (volume < config->volumeMin + 2) {
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

void Radio::printConfigToSerial() {
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
  Serial.printf("pcbVersion=%d\n", config->pcbVersion);
}

void Radio::debugModeLoop() {
  // TODO this needs updated to new status object. Maybe use what's in the status object, instead reading.
  debugLPS++;
  if (millis() - lastDebugStatusUpdate > config->debugStatusUpdateIntervalMs) {
    // Serial.printf("radio.readVolume()=%d\n", readVolume());
    // Serial.print("radio.readChannelIdx()=");
    // Serial.println(readChannelIdx());
    // Serial.printf("radio.status.channelIdx=%d\n", status.channelIdxInput);
    // Serial.printf("radio.status.playing=%d\n", status.playing);

    int lps = debugLPS / (config->debugStatusUpdateIntervalMs / 1000);
    debugLPS = 0;
    Serial.printf("lps=%d\n", lps);

    lastDebugStatusUpdate = millis();
  }
}

void Radio::handleSerialInput() {
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
      config->stationCount = doc["stationCount"] | config->stationCount;
      config->maxStationCount = doc["maxStationCount"] | config->maxStationCount;

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

      if (doc["debugMode"]) {
        debugMode = true;
        ledStatus.mDebugMode = true;
        wifiManager->setDebugOutput(true);
      }

      printConfigToSerial();
    }
  }
}

bool Radio::streamIsAdvancing() {
  /*

  THIS IS NOT WORKING AND JUST RETURNS TRUE

  TODO:
  - Check out streamDetection - source code would need edited, but that would be the best place to understand the buffer usage and last time data was received.

  */

  return true;

  // // Check if the audio->getAudioCurrentTime() is advancing, if not, set the status to match (triggering a reconnect).

  // bool retVal = true;
  // uint32_t currentTime = audio->getAudioCurrentTime();
  // // Give the stream config->streamLossDetectionWindowS * 3 to get started (otherwise this may check too soon and start a loop of reconnects)
  // if (currentTime > config->streamLossDetectionWindowS * 3 && currentTime - lastCurrentTime < config->streamLossDetectionWindowS * 0.5) {
  //   if (debugMode) {
  //     Serial.print("Connection loss detected");
  //   }
  //   retVal = false;
  // }
  // lastCurrentTime = currentTime;

  // return retVal;
}

void Radio::loop() {

  audio->loop();

  handleSerialInput();

  if (debugMode) {
    debugModeLoop();
  }

  if (millis() > lastStatusCheck + config->statusCheckIntervalMs) {

    /*                                   */
    /* Handle the WiFi connection status */
    /*                                   */

    if (WiFi.isConnected()) {
      ledStatus.clearStatusCode(RADIO_STATUS_301_WIFI_CONNECTION_LOST);
    } else {
      if (debugMode) Serial.println("Reconnecting to WiFi...");
      ledStatus.setStatusCode(RADIO_STATUS_301_WIFI_CONNECTION_LOST);
      WiFi.disconnect();
      WiFi.reconnect();

      // Sleep to allow it to reconnect before triggering another disconnect.
      delay(10000);

      return;
    }

    /*                                              */
    /* Read inputs, set computed status properties. */
    /*                                              */

    status.volumeInput = readVolume();
    status.playInput = (status.volumeInput > 0);
    status.channelIdxInput = readChannelIdx();
    status.streamAdvancing = streamIsAdvancing();  // TODO - Should this be put into the if statement? It's the only place it's used.


    // Check for changes in the channel selected.  If the channel has changed, it is not connected and the new connection is not a reconnection.
    if (status.channelIdxInput != status.channelIdxOutput) {
      status.channelIdxOutput = status.channelIdxInput;
      status.reconnecting = false;
      status.playOutput = false;
    }

    // If play is requested and a previous connection has been made, but the stream is not advancing update playOutput and specify this is a reconnect
    if (status.playInput == true && status.playOutput == true && !status.streamAdvancing) {
      status.playOutput = false;
      status.reconnecting = true;
    }

    /*                           */
    /* Handle playInput == false */
    /*                           */

    // If the play input is false, but it was playing or reconnecting last cycle => disconnect.
    if (status.playInput == false && (status.playOutput == true || status.reconnecting == true)) {
      // stop play, set status
      setDacSdMode(false);  // Turn DAC off
      audio->stopSong();

      status.playOutput = false;
      status.reconnecting = false;

      // Clear warning level and up, since it doesn't matter if a connection cannot be made. This will still allow WiFi connection errors to be displayed.
      ledStatus.setStatusCode(RADIO_STATUS_000_IDLE, LED_STATUS_200_WARNING_LEVEL);

      return;
    }

    /*                          */
    /* Handle playInput == true */
    /*                          */

    audio->setVolume(status.volumeInput);

    // Play is requested, Play is output, make sure warnings are cleared and the success status is set.
    if (status.playInput == true && status.playOutput == true) {
      ledStatus.setStatusCode(RADIO_STATUS_101_PLAYING, LED_STATUS_200_WARNING_LEVEL);
      return;
    }

    // The input says to play, but the output is not play
    if (status.playInput == true && status.playOutput == false) {

      // Set the LED status.
      if (status.reconnecting) {
        ledStatus.setStatusCode(RADIO_STATUS_251_STREAM_CONNECTION_LOST_RECONNECTING);
      } else {
        ledStatus.setStatusCode(RADIO_STATUS_051_INITIAL_STREAMING_CONNECTION, LED_STATUS_100_SUCCESS_LEVEL);
      }

      status.playOutput = connectToStream();

      if (status.playOutput) {
        status.reconnecting = false;
        ledStatus.setStatusCode(RADIO_STATUS_101_PLAYING, LED_STATUS_200_WARNING_LEVEL);
      } else {
        status.reconnecting = true;
      }
    }
  }
}