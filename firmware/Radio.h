/*

Radio contains the current state of the radio hardware, as well as some settings.

It is unaware of the Audio & LEDStatus objects.

*/
#include <Preferences.h>

#define RADIO_DEFAULT_REMOTE_LIST_URL ""
#define RADIO_DEFAULT_REMOTE_LIST false
#define RADIO_DEFAULT_HAS_CHANNEL_POT false
#define RADIO_DEFAULT_PCB_VERSION ""
#define RADIO_DEFAULT_RADIO_ID ""

#define RADIO_DEFAULT_STATION_ONE_URL "http://acc-radio.raiotech.com/mansfield.mp3"
#define RADIO_DEFAULT_STATION_TWO_URL ""
#define RADIO_DEFAULT_STATION_THREE_URL ""
#define RADIO_DEFAULT_STATION_FOUR_URL ""
#define RADIO_DEFAULT_STATION_COUNT 1

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
  bool debugMode = false;
  Preferences preferences;

  // Config
  String remoteListURL = RADIO_DEFAULT_REMOTE_LIST_URL;
  bool remoteList = RADIO_DEFAULT_REMOTE_LIST;
  String radioID = RADIO_DEFAULT_RADIO_ID;
  bool hasChannelPot = RADIO_DEFAULT_HAS_CHANNEL_POT;
  String pcbVersion = RADIO_DEFAULT_PCB_VERSION;
  String stnOneURL = RADIO_DEFAULT_STATION_ONE_URL;
  String stnTwoURL = RADIO_DEFAULT_STATION_TWO_URL;
  String stnThreeURL = RADIO_DEFAULT_STATION_THREE_URL;
  String stnFourURL = RADIO_DEFAULT_STATION_FOUR_URL;
  int stationCount = RADIO_DEFAULT_STATION_COUNT;
  int maxStationCount = 4; // This is determined by hardware
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
  remoteListURL = preferences.getString("remoteListURL", RADIO_DEFAULT_REMOTE_LIST_URL);
  remoteList = preferences.getBool("remoteList", RADIO_DEFAULT_REMOTE_LIST);
  radioID = preferences.getString("radioID", RADIO_DEFAULT_RADIO_ID);
  hasChannelPot = preferences.getBool("hasChannelPot", RADIO_DEFAULT_HAS_CHANNEL_POT);
  pcbVersion = preferences.getString("pcbVersion", RADIO_DEFAULT_PCB_VERSION);
  stnOneURL = preferences.getString("stnOneURL", RADIO_DEFAULT_STATION_ONE_URL);
  stnTwoURL = preferences.getString("stnTwoURL", RADIO_DEFAULT_STATION_TWO_URL);
  stnThreeURL = preferences.getString("stnThreeURL", RADIO_DEFAULT_STATION_THREE_URL);
  stnFourURL = preferences.getString("stnFourURL", RADIO_DEFAULT_STATION_FOUR_URL);
  stationCount = preferences.getInt("stationCount", RADIO_DEFAULT_STATION_COUNT);
  preferences.end();
}

void Radio::putConfigToPreferences() {
  preferences.begin("config", false);
  preferences.putString("remoteListURL", remoteListURL);
  preferences.putBool("remoteList", remoteList);
  preferences.putString("radioID", radioID);
  preferences.putBool("hasChannelPot", hasChannelPot);
  preferences.putString("pcbVersion", pcbVersion);
  preferences.putString("stnOneURL", stnOneURL);
  preferences.putString("stnTwoURL", stnTwoURL);
  preferences.putString("stnThreeURL", stnThreeURL);
  preferences.putString("stnFourURL", stnFourURL);
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