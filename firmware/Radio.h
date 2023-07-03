/*

Radio contains the current state of the radio hardware, as well as some settings.

It is unaware of the Audio & LEDStatus objects.

*/
#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

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

#define RADIO_DEFAULT_MAX_STATION_COUNT 4 // This is determined by hardware


WiFiClient client;
HTTPClient http;

class Radio {
  int mChannelPotPin;
  int mVolumePotPin;
  int mVolumeMin;
  int mVolumeMax;

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

public:
  Radio(int channelPotPin, int volumePotPin, int volumeMin, int volumeMax);
  void init();
  void getConfigFromPreferences();
  void putConfigToPreferences();
  int readChannelIdx();
  int readVolume();
  void setDebugMode();
  bool getConfigFromRemote();
  void debugPrintConfigToSerial();
  bool debugMode = false;
  Preferences preferences;

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

Radio::Radio(int channelPotPin, int volumePotPin, int volumeMin, int volumeMax) {
  mChannelPotPin = channelPotPin;
  mVolumePotPin = volumePotPin;
  mVolumeMin = volumeMin;
  mVolumeMax = volumeMax;
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
