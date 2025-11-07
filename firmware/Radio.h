#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Audio.h"
#include "LEDStatus.h"

/*


TODO 
  - Make sure the code is safe for millis rollover https://arduino.stackexchange.com/questions/12587/how-can-i-handle-the-millis-rollover
  - In the if statement for reconnecting, make sure the 2000 milli delay is appropriate for the audio library's connecting timeout


MANUAL TESTS:

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
#define RADIO_STATUS_450_UNABLE_TO_CONNECT_TO_WIFI_WM_ACTIVE 450
// Loop
#define RADIO_STATUS_400_WIFI_CONNECTION_LOST 400

/* WARNINGS */
// Setup
#define RADIO_STATUS_350_UNABLE_TO_CONNECT_TO_CONFIG_SERVER 350
// Loop
#define RADIO_STATUS_351_STREAM_CONNECTION_LOST_RECONNECTING 351
#define RADIO_STATUS_352_UNABLE_TO_CONNECT_WAITING_AND_TRYING_AGAIN 352

/* SUCCESS */
// Loop
#define RADIO_STATUS_201_PLAYING 201

/* INFO */
// Setup
#define RADIO_STATUS_101_RADIO_INITIALIZING 101
// Loop
#define RADIO_STATUS_102_INITIAL_STREAMING_CONNECTION 102
#define RADIO_STATUS_151_BUFFERING 151

/* LEDs OFF */
// Setup
#define RADIO_STATUS_000_BOOT_COMPLETE 0

// Loop
#define RADIO_STATUS_001_IDLE 1
#define RADIO_STATUS_002_BACKGROUND_CONFIG_RETRIEVAL 2


WiFiClient client;
HTTPClient http;

struct RadioConfig {

  // Hardware
  int pin_channel_pot = 2;
  int pin_volume_pot = 8;
  int pin_dac_sd_mode = 11;
  int pin_i2s_dout = 12;
  int pin_i2s_bclk = 13;
  int pin_i2s_lrc = 14;
  bool has_channel_pot = true;
  String pcb_version = "";
  int analog_read_resolution = 12;  // This ensures the maps used in read_channel_index and read_volume are correct.

  // Software
  int volume_min = 0;
  int volume_max = 21;
  String stn_1_url = "";
  String stn_2_url = "";
  String stn_3_url = "";
  String stn_4_url = "";
  int station_count = 1;
  int max_station_count = 4;

  // Intervals
  int debug_status_update_interval_ms = 5000;
  int status_check_interval_ms = 50;
  int wifi_disconnect_timeout_ms = 300000;      // 5 minutes
  int wifi_init_disconnect_timeout_ms = 900000; // 15 minutes (If it's in WiFi setup mode for 15 mins, restart. It can enter WiFi setup mode if there is a power outage and the radio boots before the router. )
    int reconnecting_timeout_ms = 300000;       // 5 minutes

  // Remote Config
  bool remote_config = false;
  String remote_cfg_url = "";
  String radio_id = "";
  int remote_config_background_retrieval_interval = 0;
};

class Radio {
  unsigned long m_last_status_check_ = 0;
  unsigned long m_last_remote_config_retrieved_ = 0;
  unsigned long m_last_reconnection_attempt = 0;
  unsigned long m_last_wifi_connected = 0;
  unsigned long m_last_good_connection = 0;

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

  /* Status related variables */

  // Inputs
  int m_channel_index_input = 0;
  int m_volume_input = 0;
  bool m_play_input = false;

  // Outputs
  int m_channel_index_output = 0;

  bool m_reconnecting_to_stream = false;

  // Statuses (Associated with actions)
  bool m_playing = false;  // (this is volume > 0 and connection successful) Changing volume from 0 to something greater than 0 requests the stream. Changing the volume to 0 stops the stream.
  bool m_stream_connect_established = false;

public:
  Radio(RadioConfig *radio_config, WiFiManager *myWifiManager, Audio *myAudio, LEDStatusConfig *led_status_config);
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
  RadioConfig *m_radio_config;
  WiFiManager *m_wifi_manager;
  Audio *audio;
};

Radio::Radio(RadioConfig *radio_config, WiFiManager *myWifiManager, Audio *myAudio, LEDStatusConfig *led_status_config) {
  m_radio_config = radio_config;
  m_wifi_manager = myWifiManager;
  audio = myAudio;
  m_led_status_config = led_status_config;
}

bool Radio::connect_to_stream_host() {

  set_dac_sd_mode(true);  // Turn DAC on

  String *channels[] = { &m_radio_config->stn_1_url, &m_radio_config->stn_2_url, &m_radio_config->stn_3_url, &m_radio_config->stn_4_url };
  char selected_channel_url[2048];
  channels[m_channel_index_output]->toCharArray(selected_channel_url, 2048);

  if (m_debug_mode) {
    Serial.print("selected_channel_url=");
    Serial.println(selected_channel_url);
  }

  return audio->connecttohost(selected_channel_url);
}

void Radio::get_config_from_preferences() {
  // IMPORTANT the preferences library accepts keys up to 15 characters. Larger keys can be passed and no error will be thrown, but strange things may happen.
  preferences.begin("config", false);
  m_radio_config->remote_cfg_url = preferences.getString("remote_cfg_url", m_radio_config->remote_cfg_url);
  m_radio_config->remote_config = preferences.getBool("remote_config", m_radio_config->remote_config);
  m_radio_config->remote_config_background_retrieval_interval = preferences.getInt("ret_rem_cfg_int", m_radio_config->remote_config_background_retrieval_interval);
  m_radio_config->radio_id = preferences.getString("radio_id", m_radio_config->radio_id);
  m_radio_config->has_channel_pot = preferences.getBool("has_channel_pot", m_radio_config->has_channel_pot);
  m_radio_config->pcb_version = preferences.getString("pcb_version", m_radio_config->pcb_version);
  m_radio_config->stn_1_url = preferences.getString("stn_1_url", m_radio_config->stn_1_url);
  m_radio_config->stn_2_url = preferences.getString("stn_2_url", m_radio_config->stn_2_url);
  m_radio_config->stn_3_url = preferences.getString("stn_3_url", m_radio_config->stn_3_url);
  m_radio_config->stn_4_url = preferences.getString("stn_4_url", m_radio_config->stn_4_url);
  m_radio_config->station_count = preferences.getInt("station_count", m_radio_config->station_count);
  preferences.end();
}

void Radio::put_config_to_preferences() {
  preferences.begin("config", false);
  preferences.putString("remote_cfg_url", m_radio_config->remote_cfg_url);
  preferences.putBool("remote_config", m_radio_config->remote_config);
  preferences.putInt("ret_rem_cfg_int", m_radio_config->remote_config_background_retrieval_interval);
  preferences.putString("radio_id", m_radio_config->radio_id);
  preferences.putBool("has_channel_pot", m_radio_config->has_channel_pot);
  preferences.putString("pcb_version", m_radio_config->pcb_version);
  preferences.putString("stn_1_url", m_radio_config->stn_1_url);
  preferences.putString("stn_2_url", m_radio_config->stn_2_url);
  preferences.putString("stn_3_url", m_radio_config->stn_3_url);
  preferences.putString("stn_4_url", m_radio_config->stn_4_url);
  preferences.putInt("station_count", m_radio_config->station_count);
  preferences.end();
}

bool Radio::get_config_from_remote() {
  // returns error: true|false

  // To preventing flooding, this is set regardless of the success
  m_last_remote_config_retrieved_ = millis();

  if (!m_radio_config->remote_config) {
    return false;
  }

  char has_channel_potentiometer[5];
  (m_radio_config->has_channel_pot) ? strcpy(has_channel_potentiometer, "true") : strcpy(has_channel_potentiometer, "false");
  String url = m_radio_config->remote_cfg_url
               + m_radio_config->radio_id
               + "?pcb_version=" + m_radio_config->pcb_version
               + "&firmware_version=" + FIRMWARE_VERSION
               + "&max_station_count=" + (String)m_radio_config->max_station_count
               + "&has_channel_potentiometer=" + has_channel_potentiometer;

  if (m_debug_mode) {
    Serial.print(F("Remote config: url="));
    Serial.println(url);
  }

  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.useHTTP10(true);
  http.begin(client, url);
  int code = http.GET();

  if (m_debug_mode) {
    Serial.print(F("Config HTTP Request Response Code: "));
    Serial.println(code);
  }

  if (code > 399) {
    return true;
  }

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

  m_radio_config->stn_1_url = doc["stn1URL"].as<String>();
  m_radio_config->stn_2_url = doc["stn2URL"].as<String>();
  m_radio_config->stn_3_url = doc["stn3URL"].as<String>();
  m_radio_config->stn_4_url = doc["stn4URL"].as<String>();
  m_radio_config->station_count = doc["stationCount"];
  m_radio_config->remote_config_background_retrieval_interval = doc["remote_config_background_retrieval_interval"] | m_radio_config->remote_config_background_retrieval_interval;
  put_config_to_preferences();

  if (m_debug_mode) {
    Serial.println("Sucessfully retrieved config from remote server.");
  }

  // If the stations are passed as an array, access the array like this:
  // JsonArray station_urls = doc["station_urls"].as<JsonArray>();

  http.end();

  return false;
}

int Radio::read_channel_index() {
  int channel_input = analogRead(m_radio_config->pin_channel_pot);
  channel_input = map(channel_input, 0, 4095, 0, 11);
  return m_channels_lookup_table_[m_radio_config->station_count - 1][channel_input];
}

int Radio::read_volume() {
  int volume_raw = analogRead(m_radio_config->pin_volume_pot);
  int volume = map(volume_raw, 0, 4095, m_radio_config->volume_min, m_radio_config->volume_max);
  return volume;
}

void Radio::set_dac_sd_mode(bool enable) {
  // Sets the SD mode pin that's connected to the DAC. true turns the DAC on, false turns the dac off.
  if (enable) {
    digitalWrite(m_radio_config->pin_dac_sd_mode, HIGH);
  } else {
    digitalWrite(m_radio_config->pin_dac_sd_mode, LOW);
  }
}

void Radio::init() {

  Serial.begin(115200);

  init_debug_mode();

  m_led_status.init(m_led_status_config, m_debug_mode);

  get_config_from_preferences();

  if (m_debug_mode) {
    Serial.println("DEBUG MODE ON");
    m_led_status.set_status(LED_STATUS_LEVEL_100_BLUE_INFO, LED_STATUS_MAX_CODE);
    delay(250);
    m_led_status.set_status(LED_STATUS_LEVEL_200_GREEN_SUCCESS, LED_STATUS_MAX_CODE);
    delay(250);
    m_led_status.set_status(LED_STATUS_LEVEL_300_YELLOW_WARNING, LED_STATUS_MAX_CODE);
    delay(250);
    m_led_status.set_status(LED_STATUS_LEVEL_400_RED_ERROR, LED_STATUS_MAX_CODE);
    delay(250);
    print_config_to_serial();
  }

  m_led_status.set_status(LED_STATUS_LEVEL_400_RED_ERROR, LED_STATUS_MAX_CODE);
  delay(250);
  m_led_status.set_status(LED_STATUS_LEVEL_300_YELLOW_WARNING, LED_STATUS_MAX_CODE);
  delay(250);
  m_led_status.set_status(LED_STATUS_LEVEL_200_GREEN_SUCCESS, LED_STATUS_MAX_CODE);
  delay(250);
  m_led_status.set_status(LED_STATUS_LEVEL_100_BLUE_INFO, LED_STATUS_MAX_CODE);
  delay(250);

  analogReadResolution(m_radio_config->analog_read_resolution);

  pinMode(m_radio_config->pin_dac_sd_mode, OUTPUT);

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

    // If it's been more than wifi_disconnect_timeout_ms since it's been connected to wifi, restart the esp.
    if (millis() > m_radio_config->wifi_init_disconnect_timeout_ms) {
      ESP.restart();
      return;
    }
  }

  // WiFi is connected, clear the code, if set.
  m_led_status.clear_status(RADIO_STATUS_450_UNABLE_TO_CONNECT_TO_WIFI_WM_ACTIVE);

  // Initialize Audio
  audio->setPinout(m_radio_config->pin_i2s_bclk, m_radio_config->pin_i2s_lrc, m_radio_config->pin_i2s_dout);
  audio->setVolume(0);

  // Get config from remote server
  bool error = get_config_from_remote();
  if (error) {
    // Show the error, but move on since the radio should be able to use the config stored in preferences.
    m_led_status.set_status(RADIO_STATUS_350_UNABLE_TO_CONNECT_TO_CONFIG_SERVER);
    delay(6000);
    m_led_status.clear_status(RADIO_STATUS_350_UNABLE_TO_CONNECT_TO_CONFIG_SERVER);
  }

  m_led_status.set_status(RADIO_STATUS_000_BOOT_COMPLETE);
};

void Radio::init_debug_mode() {

  // If debug mode was set to true elsewhere (as part of the setup()), don't check the volume.
  if (m_debug_mode == true) return;

  // Detect if debug mode is being requested by having the volume set to full when turned on (or reset) and then turned to 0 within 3 seconds.
  int volume = read_volume();
  if (volume > m_radio_config->volume_max - 2) {
    delay(3000);
    volume = read_volume();
    if (volume < m_radio_config->volume_min + 2) {
      m_debug_mode = true;
      return;
    };
  }
  m_debug_mode = false;
}

void Radio::print_config_to_serial() {
  Serial.print("FIRMWARE_VERSION=");
  Serial.println(FIRMWARE_VERSION);
  Serial.println("-----------");
  Serial.println("Radio Config");
  Serial.print("remote_cfg_url=");
  Serial.println(m_radio_config->remote_cfg_url);
  Serial.printf("remote_config=%d\n", m_radio_config->remote_config);
  Serial.print("radioID=");
  Serial.println(m_radio_config->radio_id);
  Serial.printf("has_channel_pot=%d\n", m_radio_config->has_channel_pot);
  Serial.print("stn_1_url=");
  Serial.println(m_radio_config->stn_1_url);
  Serial.print("stn_2_url=");
  Serial.println(m_radio_config->stn_2_url);
  Serial.print("stn_3_url=");
  Serial.println(m_radio_config->stn_3_url);
  Serial.print("stn_4_url=");
  Serial.println(m_radio_config->stn_4_url);
  Serial.printf("station_count=%d\n", m_radio_config->station_count);
  Serial.printf("max_station_count=%d\n", m_radio_config->max_station_count);
  Serial.print("pcb_version=");
  Serial.println(m_radio_config->pcb_version);
  Serial.print("remote_config_background_retrieval_interval=");
  Serial.println(m_radio_config->remote_config_background_retrieval_interval);
}

void Radio::debug_mode_loop() {
  m_debug_lps_++;
  if (millis() - m_last_debug_status_update_ > m_radio_config->debug_status_update_interval_ms) {
    int lps = m_debug_lps_ / (m_radio_config->debug_status_update_interval_ms / 1000);
    m_debug_lps_ = 0;
    Serial.printf("lps=%d\n", lps);

    m_last_debug_status_update_ = millis();

    Serial.print("Heap: ");
    Serial.print(esp_get_free_heap_size());
    Serial.print(':');
    Serial.println(heap_caps_get_largest_free_block(MALLOC_CAP_DMA));

    // These all appear to repeat the same data
    // Serial.print(':');
    // Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    // Serial.print(':');
    // Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    // Serial.print(':');
    // Serial.println(heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
  }
}

void Radio::handle_serial_input_() {
  while (Serial.available() > 0) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, Serial);
    if (!error) {

      serializeJson(doc, Serial);
      Serial.println("");

      m_radio_config->remote_cfg_url = doc["remote_cfg_url"] | m_radio_config->remote_cfg_url;
      m_radio_config->remote_config = doc["remote_config"] | m_radio_config->remote_config;
      m_radio_config->remote_config_background_retrieval_interval = doc["remote_config_background_retrieval_interval"] | m_radio_config->remote_config_background_retrieval_interval;
      m_radio_config->radio_id = doc["radio_id"] | m_radio_config->radio_id;

      // should the stuff after | be there? if it works, then leave it alone
      m_radio_config->has_channel_pot = doc["has_channel_pot"] | m_radio_config->has_channel_pot;
      m_radio_config->pcb_version = doc["pcb_version"] | m_radio_config->pcb_version;
      m_radio_config->stn_1_url = doc["stn_1_url"] | m_radio_config->stn_1_url;
      m_radio_config->stn_2_url = doc["stn_2_url"] | m_radio_config->stn_2_url;
      m_radio_config->stn_3_url = doc["stn_3_url"] | m_radio_config->stn_3_url;
      m_radio_config->stn_4_url = doc["stn_4_url"] | m_radio_config->stn_4_url;
      m_radio_config->station_count = doc["station_count"] | m_radio_config->station_count;
      m_radio_config->max_station_count = doc["max_station_count"] | m_radio_config->max_station_count;

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


  /*

Try 
  - downgrade audio library? Check libs on both computers??? Maybe laptop has a bad lib?

  - serial only in debug mode, as was before (not even enabled)
  - Don't update config?? But it was playing when it was updated....
  - decrease size of arduino json - and just do small updates at once, no need for large. just one k/v at a time...
  - decrease number of strings? Esp any string manipulation? Watch the scope of the strings

  - Big drop in heap_caps_get_largest_free_block(MALLOC_CAP_DMA) when changing channels. - it uses 4 strings...

  - does reconnect loop interfer with checking wifi??   MAYBE BLOCKING IS BAD???

*/


  audio->loop();

  while (Serial.available() > 0) {
    handle_serial_input_();
  }

  if (m_debug_mode) {
    debug_mode_loop();
  }

  if (millis() > m_last_status_check_ + m_radio_config->status_check_interval_ms) {

    m_last_status_check_ = millis();

    /*                                   */
    /* Handle the WiFi connection status */
    /*                                   */

    if (WiFi.isConnected()) {
      m_led_status.clear_status(RADIO_STATUS_400_WIFI_CONNECTION_LOST);
      m_last_wifi_connected = millis();
    } else {
      if (m_debug_mode) Serial.println("Reconnecting to WiFi...");
      m_led_status.set_status(RADIO_STATUS_400_WIFI_CONNECTION_LOST);

      // If it's been more than wifi_disconnect_timeout_ms since it's been connected to wifi, restart the esp.
      if (millis() > m_last_wifi_connected + m_radio_config->wifi_disconnect_timeout_ms) {
        ESP.restart();
        return;
      }
      WiFi.reconnect();
      return;
    }

    /*                                              */
    /* Read inputs, set computed status properties. */
    /*                                              */

    m_volume_input = read_volume();
    m_play_input = (m_volume_input > 0);
    m_channel_index_input = read_channel_index();

    // Check for changes in the channel selected.  If the channel has changed, it is not connected and the new connection is not a reconnection.
    if (m_channel_index_input != m_channel_index_output) {
      m_channel_index_output = m_channel_index_input;
      m_reconnecting_to_stream = false;
      m_stream_connect_established = false;
      audio->stopSong();
    }

    // If play is requested and the stream had previously been connected, then the stream is reconnecting.
    if (m_play_input && m_stream_connect_established && !stream_is_running()) {
      m_stream_connect_established = false;
      m_reconnecting_to_stream = true;
    }

    /*                           */
    /* Handle playInput == false */
    /*                           */

    if (!m_play_input) {
      // If the play input is false, but it was playing or reconnecting last cycle => disconnect.
      if (m_stream_connect_established || stream_is_running() || m_reconnecting_to_stream) {
        // stop play, set status
        m_stream_connect_established = false;
        m_reconnecting_to_stream = false;
        set_dac_sd_mode(false);  // Turn DAC off
        audio->stopSong();
      }
      // Clear warning level and up, since it doesn't matter if a connection cannot be made. This will still allow WiFi connection errors to be displayed.
      m_led_status.set_status(RADIO_STATUS_001_IDLE, LED_STATUS_LEVEL_400_RED_ERROR);

      // Check the last time the configuration was downloaded, and download.
      if (m_radio_config->remote_config && m_radio_config->remote_config_background_retrieval_interval > 0 && millis() - m_last_remote_config_retrieved_ > m_radio_config->remote_config_background_retrieval_interval) {
        m_led_status.set_status(RADIO_STATUS_002_BACKGROUND_CONFIG_RETRIEVAL);
        get_config_from_remote();
      }

      return;
    }

    /*                          */
    /* Handle playInput == true */
    /*                          */

    audio->setVolume(m_volume_input);

    // Play is requested, There is a connection to the host, and the stream is running. Make sure warnings are cleared and the success status is set.
    if (m_play_input == true && stream_is_running()) {
      m_reconnecting_to_stream = false;
      m_stream_connect_established = true;
      m_last_good_connection = millis();

      if (audio->inBufferFilled() > 0) {
        // Once there is something in the buffer (ie: when more than just a connection to the host is made) update the status
        m_led_status.set_status(RADIO_STATUS_201_PLAYING, LED_STATUS_LEVEL_400_RED_ERROR);
      } else {
        // If the buffer is still at 0, then blink blue.
        m_led_status.set_status(RADIO_STATUS_151_BUFFERING, LED_STATUS_LEVEL_400_RED_ERROR);
      }
      return;
    }

    // The input says to play, but there is no connection to the stream host.
    if (m_play_input == true && !stream_is_running()) {

      // Set the LED status.
      if (m_reconnecting_to_stream) {
        m_led_status.set_status(RADIO_STATUS_351_STREAM_CONNECTION_LOST_RECONNECTING);
      } else {
        m_led_status.set_status(RADIO_STATUS_102_INITIAL_STREAMING_CONNECTION, LED_STATUS_LEVEL_300_YELLOW_WARNING);
      }

      // If it is a reconnect, only attempt if more than 5 seconds after last reconnecting attempt - giving 2 seconds to attempt a connection and fill the streaming buffer (making stream_is_running() true).
      if (m_reconnecting_to_stream && millis() - 5000 > m_last_reconnection_attempt) {
        m_last_reconnection_attempt = millis();

        // TODO the amount of time to wait since the last reconnection should relate to the audio library's timeout for connections. 2000 millis is just what seems right.
        // Stop any previous connection
        audio->stopSong();
        // Connect
        connect_to_stream_host();
      }

      // If this isn't a reconnect, then connect
      if (!m_reconnecting_to_stream) {
        connect_to_stream_host();
      }

      // If the stream is reconnecting and the stream has not been seen for more than the time out, restart the esp
      // This could count retries in the block above? But this works as well.
      if (m_reconnecting_to_stream && m_last_good_connection + m_radio_config->reconnecting_timeout_ms < millis()) {
        ESP.restart();
        return;
      }
    }
  }
}