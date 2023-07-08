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


#define RADIO_DEFAULT_REMOTE_LIST_URL ""
#define RADIO_DEFAULT_REMOTE_LIST false
#define RADIO_DEFAULT_HAS_CHANNEL_POT false
#define RADIO_DEFAULT_PCB_VERSION ""
#define RADIO_DEFAULT_RADIO_ID ""

#define RADIO_DEFAULT_STATION_ONE_URL ""
#define RADIO_DEFAULT_STATION_TWO_URL ""
#define RADIO_DEFAULT_STATION_THREE_URL ""
#define RADIO_DEFAULT_STATION_FOUR_URL ""
#define RADIO_DEFAULT_STATION_COUNT 1

#define RADIO_DEFAULT_MAX_STATION_COUNT 4  // This is determined by hardware

#define RADIO_DEBUG_STATUS_UPDATE_INTERVAL_MS 5000

#define RADIO_VOLUME_CHANNEL_READ_INTERVAL_MS 50

#define RADIO_CURRENT_TIME_CHECK_INTERVAL_S 3



WiFiClient client;
HTTPClient http;

struct RadioStatus {

  // This will be replaced by the below
  // int channelIdx;
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
  bool wifiConnected = false;      // Satus of WiFi after booting. (The initial connection is handled by WifiManager, and the LEDs should be set there.)
  bool streamConnecting = false;   // This is set before connecting to a new stream (whether starting or when changing stations)
  bool streamPlaying = false;      // (this is volume > 0 and connection successful) Changing volume from 0 to something greater than 0 requests the stream. Changing the volume to 0 stops the stream.
  bool streamIsAdvancing = false;  // This is set by checking if the number of seconds played has advanced

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
  int mChannelPotPin;
  int mVolumePotPin;
  int mVolumeMin;
  int mVolumeMax;
  unsigned long lastVolumeChannelRead = 0;
  unsigned long lastCurrentTimeCheck = 0;
  uint32_t lastCurrentTime = 0;

  /*

    Split the fader up into 13 parts (0-12), which correpsond with 3 channels.

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
  Radio(int channelPotPin, int volumePotPin, int volumeMin, int volumeMax, WiFiManager *myWifiManager, Audio *myAudio);
  void init();
  void getConfigFromPreferences();
  void putConfigToPreferences();
  int readChannelIdx();
  int readVolume();
  void setDebugMode();
  bool getConfigFromRemote();
  void debugPrintConfigToSerial();
  void setStatusCode();
  void loop();
  void debugModeLoop();
  bool debugMode = false;
  RadioStatus status;
  Preferences preferences;
  LEDStatus ledStatus;
  WiFiManager *wifiManager;
  Audio *audio;

  // Config
  String remoteConfigURL = RADIO_DEFAULT_REMOTE_LIST_URL;
  bool remoteConfig = RADIO_DEFAULT_REMOTE_LIST;
  String radioID = RADIO_DEFAULT_RADIO_ID;
  bool hasChannelPot = RADIO_DEFAULT_HAS_CHANNEL_POT;
  String pcbVersion = RADIO_DEFAULT_PCB_VERSION;
  String stn1URL = RADIO_DEFAULT_STATION_ONE_URL;
  String stn2URL = RADIO_DEFAULT_STATION_TWO_URL;
  String stn3URL = RADIO_DEFAULT_STATION_THREE_URL;
  String stn4URL = RADIO_DEFAULT_STATION_FOUR_URL;
  int stationCount = RADIO_DEFAULT_STATION_COUNT;
  int maxStationCount = RADIO_DEFAULT_MAX_STATION_COUNT;
};

Radio::Radio(int channelPotPin, int volumePotPin, int volumeMin, int volumeMax, WiFiManager *myWifiManager, Audio *myAudio) {
  mChannelPotPin = channelPotPin;
  mVolumePotPin = volumePotPin;
  mVolumeMin = volumeMin;
  mVolumeMax = volumeMax;
  wifiManager = myWifiManager;
  audio = myAudio;
  status.playing = false;
  status.channelIdx = 0;
}

void Radio::getConfigFromPreferences() {
  // IMPORTANT the preferences library accepts keys up to 15 characters. Larger keys can be passed and no error will be thrown, but strange things may happen.
  preferences.begin("config", false);
  remoteConfigURL = preferences.getString("remoteConfigURL", RADIO_DEFAULT_REMOTE_LIST_URL);
  remoteConfig = preferences.getBool("remoteConfig", RADIO_DEFAULT_REMOTE_LIST);
  radioID = preferences.getString("radioID", RADIO_DEFAULT_RADIO_ID);
  hasChannelPot = preferences.getBool("hasChannelPot", RADIO_DEFAULT_HAS_CHANNEL_POT);
  pcbVersion = preferences.getString("pcbVersion", RADIO_DEFAULT_PCB_VERSION);
  stn1URL = preferences.getString("stn1URL", RADIO_DEFAULT_STATION_ONE_URL);
  stn2URL = preferences.getString("stn2URL", RADIO_DEFAULT_STATION_TWO_URL);
  stn3URL = preferences.getString("stn3URL", RADIO_DEFAULT_STATION_THREE_URL);
  stn4URL = preferences.getString("stn4URL", RADIO_DEFAULT_STATION_FOUR_URL);
  stationCount = preferences.getInt("stationCount", RADIO_DEFAULT_STATION_COUNT);
  preferences.end();
}

void Radio::putConfigToPreferences() {
  preferences.begin("config", false);
  preferences.putString("remoteConfigURL", remoteConfigURL);
  preferences.putBool("remoteConfig", remoteConfig);
  preferences.putString("radioID", radioID);
  preferences.putBool("hasChannelPot", hasChannelPot);
  preferences.putString("pcbVersion", pcbVersion);
  preferences.putString("stn1URL", stn1URL);
  preferences.putString("stn2URL", stn2URL);
  preferences.putString("stn3URL", stn3URL);
  preferences.putString("stn4URL", stn4URL);
  preferences.putInt("stationCount", stationCount);
  preferences.end();
}

int Radio::readChannelIdx() {
  int channelInput = analogRead(mChannelPotPin);
  channelInput = map(channelInput, 0, 4095, 0, 11);
  return mChannelsLookupTable[stationCount - 1][channelInput];
}

int Radio::readVolume() {
  int volumeRaw = analogRead(mVolumePotPin);
  int volume = map(volumeRaw, 0, 4095, mVolumeMin, mVolumeMax);
  return volume;
}

void Radio::init() {

  // This makes sure the maps used in readChannelIdx and readVolume are correct.
  analogReadResolution(12);

  getConfigFromPreferences();
  setDebugMode();

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
};

void Radio::setDebugMode() {
  int volume = readVolume();
  if (volume == mVolumeMax) {
    delay(3000);
    volume = readVolume();
    if (volume == mVolumeMin) {
      debugMode = true;
      return;
    };
  }
  debugMode = false;
}

bool Radio::getConfigFromRemote() {
  // returns error: true|false
  if (!remoteConfig) {
    return false;
  }

  char hasPot[5];
  (hasChannelPot) ? strcpy(hasPot, "true") : strcpy(hasPot, "false");
  String url = remoteConfigURL
               + radioID
               + "?pcb_version=" + pcbVersion
               + "&firmware_version=" + FIRMWARE_VERSION
               + "&max_station_count=" + (String)maxStationCount
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

  stn1URL = doc["stn1URL"].as<String>();
  stn2URL = doc["stn2URL"].as<String>();
  stn3URL = doc["stn3URL"].as<String>();
  stn4URL = doc["stn4URL"].as<String>();
  stationCount = doc["stationCount"];
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
  Serial.println(remoteConfigURL);
  Serial.printf("remoteConfig=%d\n", remoteConfig);
  Serial.printf("radioID=%s\n", radioID);
  Serial.printf("hasChannelPot=%d\n", hasChannelPot);
  Serial.print("stn1URL=");
  Serial.println(stn1URL);
  Serial.print("stn2URL=");
  Serial.println(stn2URL);
  Serial.print("stn3URL=");
  Serial.println(stn3URL);
  Serial.print("stn4URL=");
  Serial.println(stn4URL);
  Serial.printf("stationCount=%d\n", stationCount);
  Serial.printf("maxStationCount=%d\n", maxStationCount);
}

void Radio::debugModeLoop() {
  debugLPS++;
  debugHandleSerialInput();
  if (millis() - lastDebugStatusUpdate > RADIO_DEBUG_STATUS_UPDATE_INTERVAL_MS) {
    Serial.printf("radio.readVolume()=%d\n", readVolume());
    Serial.print("radio.readChannelIdx()=");
    Serial.println(readChannelIdx());
    Serial.printf("radio.status.channelIdx=%d\n", status.channelIdx);
    Serial.printf("radio.status.playing=%d\n", status.playing);

    int lps = debugLPS / (RADIO_DEBUG_STATUS_UPDATE_INTERVAL_MS / 1000);
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

      remoteConfigURL = doc["remoteConfigURL"] | remoteConfigURL;
      remoteConfig = doc["remoteConfig"] | remoteConfig;
      radioID = doc["radioID"] | radioID;

      // should the stuff after | be there? if it works, then leave it alone
      hasChannelPot = doc["hasChannelPot"] | hasChannelPot;
      pcbVersion = doc["pcbVersion"] | pcbVersion;
      stn1URL = doc["stn1URL"] | stn1URL;
      stn2URL = doc["stn2URL"] | stn2URL;
      stn3URL = doc["stn3URL"] | stn3URL;
      stn4URL = doc["stn4URL"] | stn4URL;

      if (doc["stationCount"]) {
        stationCount = doc["stationCount"];
      }

      /* This is currently limited by the lookup table in the ChannelPot object */
      // config.maxStationCount = doc["maxStationCount"];

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
  // copy and paste everything over here and get it to compile again, lol
  if (debugMode) {
    debugModeLoop();
  }

  // Check if the WiFi has disconnected
  checkWiFiDisconnect();

  if (millis() > lastVolumeChannelRead + RADIO_VOLUME_CHANNEL_READ_INTERVAL_MS) {

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

      // start play, set status.playing
      ledStatus.setStatus(LED_STATUS_INFO_BLINKING);

      String channels[] = { stn1URL, stn2URL, stn3URL, stn4URL };
      char selectedChannelURL[2048];
      channels[status.channelIdx].toCharArray(selectedChannelURL, 2048);
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
      status.playing = false;
      audio->stopSong();
      ledStatus.setStatus(LED_STATUS_LIGHTS_OFF);
    }

    // Check if the audio->getAudioCurrentTime() is advancing, if not, set the status to match (triggering a reconnect).
    // TODO: RADIO_CURRENT_TIME_CHECK_INTERVAL_S should be renamed! It's not a timeout - more of a time in
    if (status.playing && millis() > lastCurrentTimeCheck + RADIO_CURRENT_TIME_CHECK_INTERVAL_S * 1000) {
      lastCurrentTimeCheck = millis();
      uint32_t currentTime = audio->getAudioCurrentTime();

      // Give the stream RADIO_CURRENT_TIME_CHECK_INTERVAL_S * 2 to get started (otherwise this may check too soon and start a loop of reconnects)
      if (currentTime > RADIO_CURRENT_TIME_CHECK_INTERVAL_S * 2 && currentTime - lastCurrentTime < RADIO_CURRENT_TIME_CHECK_INTERVAL_S * 0.9) {
        status.playing = false;
        lastCurrentTime = currentTime;
        if (debugMode) {
          Serial.print("Connection drop detected, triggering a reconnect.");
        }
      }
    }
  }
}
