#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Audio.h"
#include "LEDStatus.h"

/*

TODO:

  Retrieve the config after 12 hours, if the radio is idle.

  Document Programming Flow:
    1. Creating the firmware file.
      1. Use Sketch->Export Compiled Binary to create firmware that uses the settings from Tools. (USB CDC, etc)
      2. Install this on a box and connect the box to a temporary WiFi network, then download the final firmware
    2. Writing the firmware.
      1. Using an adapter, write the firmware to the board before install in a box with esptool.py. This way it will be prepared to receive firmware and serial over the USB port
      2. Board is installed in box.
      3. (Optional) firmware is updated
      4. Python script generates an ID, sends it via serial, prints label to place on box. (Radio is added to database???)

TESTS:

  Setup:
    - No WiFi Network
      - [ ] Blink Red
    - No config server
      - [ ] Blink Yellow and continue

  Loop:  
    - WiFi Loss (Restart router)
      - [ ] LED should be red, device should reconnect when WiFi turns back on
    - Connection is made before stream has started
      - [ ] LED should be yellow
      - [ ] Radio should connect when stream comes on
    - Connection lost after stream started
      - LED should be yellow
        - [ ] When connection becomes a 404
        - [ ] When server cannot be reached (timeout)
      - Radio should connect when stream comes on
        - [ ] When connection becomes a 404
        - [ ] When server cannot be reached (timeout)
    - Channel Change
      - [ ] Channel should change
      - [ ] LED should turn blue until it does.

*/

/* ERRORS */
// Setup
#define RADIO_STATUS_350_UNABLE_TO_CONNECT_TO_WIFI_WM_ACTIVE 350
// Loop
#define RADIO_STATUS_300_WIFI_CONNECTION_LOST 300

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
#define RADIO_STATUS_002_INITIAL_STREAMING_CONNECTION 2
#define RADIO_STATUS_051_BUFFERING 51

// Special Idle Status, which turns LEDs off
#define RADIO_STATUS_NEGATIVE_1_IDLE -100

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
  int analogReadResolution = 12;  // This ensures the maps used in read_channel_index and read_volume are correct.

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

  // Inputs
  int channelIdxInput = 0;
  int volumeInput = 0;
  bool playInput = false;

  // Outputs
  int channel_index_output = 0;

  bool reconnectingToStream = false;

  // Statuses (Associated with actions)
  bool playing = false;  // (this is volume > 0 and connection successful) Changing volume from 0 to something greater than 0 requests the stream. Changing the volume to 0 stops the stream.
  bool streamConnectEstablished = false;
};

class Radio {
  unsigned long m_last_status_check_ = 0;

  /*

    Example: Split the fader up into 13 parts (0-12), which correspond with 3 channels.

    0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12
    |    3    |              2            |      1      |
    The channels at the sides of potentiometer need less room since they locate easily by butting up against the end of the fader.

  */
  int m_channels_lookup_table_[4][12] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2 },
    { 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3 }
  };

  void handle_debug_mode_();
  void handle_serial_input_();

  // Debugging. If m_debug_mode is false, these are unused.
  unsigned long m_last_debug_status_update_ = 0;
  uint32_t m_debug_lps_ = 0;  // Loops per second

public:
  Radio(RadioConfig *config, WiFiManager *myWifiManager, Audio *myAudio, LEDStatusConfig *led_status_config);
  void init();
  void get_config_from_preferences();
  void put_config_to_preferences();
  int read_channel_index();
  int read_volume();
  void init_debug_mode();
  bool get_config_from_remote();
  void print_config_to_serial();
  void set_status_code();
  void loop();
  void debug_mode_loop();
  void set_dac_sd_mode(bool enable);
  bool connect_to_stream_host();
  bool stream_is_running();
  bool m_debug_mode = false;
  Preferences preferences;
  LEDStatus m_led_status;
  LEDStatusConfig *m_led_status_config;
  RadioStatus status;
  RadioConfig *config;
  WiFiManager *m_wifi_manager;
  Audio *audio;
};

Radio::Radio(RadioConfig *myConfig, WiFiManager *myWifiManager, Audio *myAudio, LEDStatusConfig *led_status_config) {
  config = myConfig;
  m_wifi_manager = myWifiManager;
  audio = myAudio;
  m_led_status_config = led_status_config;
}

bool Radio::connect_to_stream_host() {

  set_dac_sd_mode(true);  // Turn DAC on

  String *channels[] = { &config->stn1URL, &config->stn2URL, &config->stn3URL, &config->stn4URL };
  char selected_channel_url[2048];
  channels[status.channel_index_output]->toCharArray(selected_channel_url, 2048);

  if (m_debug_mode) {
    Serial.print("selected_channel_url=");
    Serial.println(selected_channel_url);
  }

  return audio->connecttohost(selected_channel_url);
}

void Radio::get_config_from_preferences() {
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

void Radio::put_config_to_preferences() {
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

int Radio::read_channel_index() {
  int channel_input = analogRead(config->pinChannelPot);
  channel_input = map(channel_input, 0, 4095, 0, 11);
  return m_channels_lookup_table_[config->stationCount - 1][channel_input];
}

int Radio::read_volume() {
  int volume_raw = analogRead(config->pinVolumePot);
  int volume = map(volume_raw, 0, 4095, config->volumeMin, config->volumeMax);
  return volume;
}

void Radio::set_dac_sd_mode(bool enable) {
  // Sets the SD mode pin that's connected to the DAC. true turns the DAC on, false turns the dac off.
  if (enable) {
    digitalWrite(config->pinDacSdMode, HIGH);
  } else {
    digitalWrite(config->pinDacSdMode, LOW);
  }
}

void Radio::init() {

  Serial.begin(115200);

  init_debug_mode();

  m_led_status.init(m_led_status_config, m_debug_mode);

  get_config_from_preferences();

  if (m_debug_mode) {
    Serial.println("DEBUG MODE ON");
    m_led_status.set_status(LED_STATUS_LEVEL_000_BLUE_INFO, LED_STATUS_MAX_CODE);
    delay(250);
    m_led_status.set_status(LED_STATUS_LEVEL_100_GREEN_SUCCESS, LED_STATUS_MAX_CODE);
    delay(250);
    m_led_status.set_status(LED_STATUS_LEVEL_200_YELLOW_WARNING, LED_STATUS_MAX_CODE);
    delay(250);
    m_led_status.set_status(LED_STATUS_LEVEL_300_RED_ERROR, LED_STATUS_MAX_CODE);
    delay(250);
    print_config_to_serial();
  }

  m_led_status.set_status(LED_STATUS_LEVEL_300_RED_ERROR, LED_STATUS_MAX_CODE);
  delay(250);
  m_led_status.set_status(LED_STATUS_LEVEL_200_YELLOW_WARNING, LED_STATUS_MAX_CODE);
  delay(250);
  m_led_status.set_status(LED_STATUS_LEVEL_100_GREEN_SUCCESS, LED_STATUS_MAX_CODE);
  delay(250);
  m_led_status.set_status(LED_STATUS_LEVEL_000_BLUE_INFO, LED_STATUS_MAX_CODE);
  delay(250);

  analogReadResolution(config->analogReadResolution);

  pinMode(config->pinDacSdMode, OUTPUT);

  // Initialize WiFi/WiFiManager
  WiFi.mode(WIFI_STA);

  // Example for setting a MAC address - useful for networks which require a registration and then filter by MAC address. Use a laptop to log in to the network (adding the laptop's MAC to the whitelist) and then set the radio's MAC address to match the laptop's address.
  // This needs to come after WiFi.mode(WIFI_STA);
  // uint8_t mac_address[] = { 0x08, 0x71, 0x90, 0x89, 0x85, 0x87 };
  // esp_wifi_set_mac(WIFI_IF_STA, mac_address);

  m_wifi_manager->setDebugOutput(m_debug_mode);
  m_wifi_manager->setConfigPortalBlocking(false);
  m_wifi_manager->autoConnect("Radio Setup");
  while (!WiFi.isConnected()) {
    // While the WiFi manager is active, handle its loop, as well as serial input.
    m_wifi_manager->process();
    handle_serial_input_();
  }

  // WiFi is connected, clear the code, if set.
  m_led_status.clear_status(RADIO_STATUS_350_UNABLE_TO_CONNECT_TO_WIFI_WM_ACTIVE);

  // Initialize Audio
  audio->setPinout(config->pinI2sBclk, config->pinI2sLrc, config->pinI2sDout);
  audio->setVolume(0);

  // Get config from remote server
  bool error = get_config_from_remote();
  if (error) {
    // Show the error, but move on since the radio should be able to use the config stored in preferences.
    m_led_status.set_status(RADIO_STATUS_250_UNABLE_TO_CONNECT_TO_CONFIG_SERVER);
    delay(6000);
    m_led_status.clear_status(RADIO_STATUS_250_UNABLE_TO_CONNECT_TO_CONFIG_SERVER);
  }

  m_led_status.set_status(RADIO_STATUS_NEGATIVE_1_IDLE);
};

void Radio::init_debug_mode() {

  // If debug mode was set to true elsewhere (as part of the setup()), don't check the volume.
  if (m_debug_mode == true) return;

  // Detect if debug mode is being requested by having the volume set to full when turned on (or reset) and then turned to 0 within 3 seconds.
  int volume = read_volume();
  if (volume > config->volumeMax - 2) {
    delay(3000);
    volume = read_volume();
    if (volume < config->volumeMin + 2) {
      m_debug_mode = true;
      return;
    };
  }
  m_debug_mode = false;
}

bool Radio::get_config_from_remote() {
  // returns error: true|false
  if (!config->remoteConfig) {
    return false;
  }

  char has_channel_potentiometer[5];
  (config->hasChannelPot) ? strcpy(has_channel_potentiometer, "true") : strcpy(has_channel_potentiometer, "false");
  String url = config->remoteConfigURL
               + config->radioID
               + "?pcb_version=" + config->pcbVersion
               + "&firmware_version=" + FIRMWARE_VERSION
               + "&max_station_count=" + (String)config->maxStationCount
               + "&has_channel_potentiometer=" + has_channel_potentiometer;

  if (m_debug_mode) {
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
    if (m_debug_mode) {
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
  put_config_to_preferences();

  if (m_debug_mode) {
    Serial.println("Sucessfully retrieved config from remote server.");
  }

  // If the stations are passed as an array, access the array like this:
  // JsonArray station_urls = doc["station_urls"].as<JsonArray>();

  http.end();

  return false;
}

void Radio::print_config_to_serial() {
  Serial.print("FIRMWARE_VERSION=");
  Serial.println(FIRMWARE_VERSION);
  Serial.println("-----------");
  Serial.println("Radio Config");
  Serial.print("remoteConfigURL=");
  Serial.println(config->remoteConfigURL);
  Serial.printf("remoteConfig=%d\n", config->remoteConfig);
  Serial.print("radioID=");
  Serial.println(config->radioID);
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
  Serial.print("pcbVersion=");
  Serial.println(config->pcbVersion);
}

void Radio::debug_mode_loop() {
  // TODO this needs updated to new status object. Maybe use what's in the status object, instead reading.
  m_debug_lps_++;
  if (millis() - m_last_debug_status_update_ > config->debugStatusUpdateIntervalMs) {
    // Serial.printf("radio.read_volume()=%d\n", read_volume());
    // Serial.print("radio.read_channel_index()=");
    // Serial.println(read_channel_index());
    // Serial.printf("radio.status.channelIdx=%d\n", status.channelIdxInput);
    // Serial.printf("radio.status.playing=%d\n", status.playing);

    int lps = m_debug_lps_ / (config->debugStatusUpdateIntervalMs / 1000);
    m_debug_lps_ = 0;
    Serial.printf("lps=%d\n", lps);

    m_last_debug_status_update_ = millis();
  }
}

void Radio::handle_serial_input_() {
  while (Serial.available() > 0) {
    DynamicJsonDocument doc(4096);
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

      put_config_to_preferences();

      if (doc["clear_preferences"]) {
        preferences.begin("config", false);
        bool cleared = preferences.clear();
        Serial.println("Clearing preferences, restarting for this to take effect.");
        preferences.end();
        ESP.restart();
      }

      if (doc["reset_wifi"]) {
        m_wifi_manager->resetSettings();
        Serial.println("Executing m_wifi_manager.resetSettings(), restarting for this to take effect.");
        ESP.restart();
      }

      if (doc["debug_mode"]) {
        m_debug_mode = true;
        m_led_status.set_debug(true);
        m_wifi_manager->setDebugOutput(true);
      }

      if (doc["ssid"]) {
        String ssid = doc["ssid"];
        String pass = doc["pass"];
        WiFi.begin(ssid.c_str(), pass.c_str(), 0, NULL, true);
      }

      if (doc["restart_esp"]) {
        ESP.restart();
      }

      print_config_to_serial();
    }
  }
}

bool Radio::stream_is_running() {

  /*

  stream_is_running() returns audio->m_f_running

  audio->isRunning() will reflect any timeouts or 404s:

  Timeouts:
    - connecttohost() will set m_f_running to true after connecting to the host (even if they host returns a 404)
    - loop > processWebStream > streamDetection checks the buffer. If no data has been received in 3 seconds, it executes connecttohost()
    - connecttohost() executes setDefaults(), which sets m_f_running=false

  404s:
    - loop > parseHttpResponseHeader checks if the status code is > 310, if so it executes stopSong(), which sets m_f_running to false.

  */

  return audio->isRunning();
}

void Radio::loop() {

  audio->loop();

  handle_serial_input_();

  if (m_debug_mode) {
    debug_mode_loop();
  }

  if (millis() > m_last_status_check_ + config->statusCheckIntervalMs) {

    /*                                   */
    /* Handle the WiFi connection status */
    /*                                   */

    if (WiFi.isConnected()) {
      m_led_status.clear_status(RADIO_STATUS_300_WIFI_CONNECTION_LOST);
    } else {
      if (m_debug_mode) Serial.println("Reconnecting to WiFi...");
      m_led_status.set_status(RADIO_STATUS_300_WIFI_CONNECTION_LOST);
      WiFi.disconnect();
      WiFi.reconnect();

      // Sleep to allow it to reconnect before triggering another disconnect.
      delay(10000);

      return;
    }

    /*                                              */
    /* Read inputs, set computed status properties. */
    /*                                              */

    status.volumeInput = read_volume();
    status.playInput = (status.volumeInput > 0);
    status.channelIdxInput = read_channel_index();

    // Check for changes in the channel selected.  If the channel has changed, it is not connected and the new connection is not a reconnection.
    if (status.channelIdxInput != status.channel_index_output) {
      status.channel_index_output = status.channelIdxInput;
      status.reconnectingToStream = false;
      status.streamConnectEstablished = false;
      audio->stopSong();
    }

    // If play is requested and the stream had previously been connected, then the stream is reconnecting.
    if (status.playInput && status.streamConnectEstablished && !stream_is_running()) {
      status.streamConnectEstablished = false;
      status.reconnectingToStream = true;
    }

    /*                           */
    /* Handle playInput == false */
    /*                           */

    if (!status.playInput) {
      // If the play input is false, but it was playing or reconnecting last cycle => disconnect.
      if (status.streamConnectEstablished || stream_is_running() || status.reconnectingToStream) {
        // stop play, set status
        status.streamConnectEstablished = false;
        status.reconnectingToStream = false;
        set_dac_sd_mode(false);  // Turn DAC off
        audio->stopSong();
      }
      // Clear warning level and up, since it doesn't matter if a connection cannot be made. This will still allow WiFi connection errors to be displayed.
      m_led_status.set_status(RADIO_STATUS_NEGATIVE_1_IDLE, LED_STATUS_LEVEL_300_RED_ERROR);
      return;
    }

    /*                          */
    /* Handle playInput == true */
    /*                          */

    audio->setVolume(status.volumeInput);

    // Play is requested, There is a connection to the host, and the stream is running. Make sure warnings are cleared and the success status is set.
    if (status.playInput == true && stream_is_running()) {
      status.reconnectingToStream = false;
      status.streamConnectEstablished = true;

      if (audio->inBufferFilled() > 0) {
        // Once there is something in the buffer (ie: when more than just a connection to the host is made) update the status
        m_led_status.set_status(RADIO_STATUS_101_PLAYING, LED_STATUS_LEVEL_300_RED_ERROR);
      } else {
        // If the buffer is still at 0, then blink blue.
        m_led_status.set_status(RADIO_STATUS_051_BUFFERING, LED_STATUS_LEVEL_300_RED_ERROR);
      }
      return;
    }

    // The input says to play, but there is no connection to the stream host.
    if (status.playInput == true && !stream_is_running()) {

      // Set the LED status.
      if (status.reconnectingToStream) {
        m_led_status.set_status(RADIO_STATUS_251_STREAM_CONNECTION_LOST_RECONNECTING);
        // Delay a few seconds before reconnecting so as not to overwhelm a server.
        // TODO - make this not blocking
        delay(5000);
      } else {
        m_led_status.set_status(RADIO_STATUS_002_INITIAL_STREAMING_CONNECTION, LED_STATUS_LEVEL_200_YELLOW_WARNING);
      }

      connect_to_stream_host();
    }
  }
}