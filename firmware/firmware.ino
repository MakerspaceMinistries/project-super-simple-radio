/*

Example serial messages for configuring the radio. It is too large to send all at once, so it is broken into parts.

{
  "remote_cfg_url":"http://config.acc-radio.raiotech.com/api/v1/radios/device_interface/v1.0/",
  "radio_id":"test-radio",
  "pcb_version":"v1-USMX.beta-1",
  "remote_config":true,
  "has_channel_pot":true,
  "station_count":4
}

{ 
  "remote_config_background_retrieval_interval": 43200000 
}

{
  "stn_1_url":"http://acc-radio.raiotech.com/connect-test.mp3",
  "stn_2_url":"http://acc-radio.raiotech.com/connect-test.mp3"
}

{
  "stn_3_url":"http://acc-radio.raiotech.com/connect-test.mp3",
  "stn_4_url":"http://acc-radio.raiotech.com/connect-test.mp3"
}

Example message for resetting the stored preferences and restarting the ESP.
{"clear_preferences": true}

States: 
  - Green               LED_STATUS_SUCCESS              success
  - Green (blinking)    LED_STATUS_SUCCESS_BLINKING     success (blinking)
  - Blue                LED_STATUS_INFO                 info
  - Blue (blinking)     LED_STATUS_INFO_BLINKING        info (blinking)
  - Yellow              LED_STATUS_WARNING              warning
  - Yellow              LED_STATUS_WARNING_BLINKING     warning (blinking)
  - Red                 LED_STATUS_ERROR                error
  - Red (blinking)      LED_STATUS_ERROR_BLINKING       error (blinking)

Board:
  esp32 2.0.9
    IMPORTANT 2.0.13 Doesn't work. It causes an exception when connecting to a stream:
      Decoding stack results
      0x42013a7c:  is in WiFiClient::write(unsigned char const*, unsigned int) (/home/jake/.arduino15/packages/esp32/hardware/esp32/2.0.9/libraries/WiFi/src/WiFiClient.cpp:400).
      0x42013b85: WiFiClient::connected() at /home/jake/.arduino15/packages/esp32/hardware/esp32/2.0.9/libraries/WiFi/src/WiFiClient.cpp:535
      0x42022691: Audio::setDefaults() at /home/jake/Arduino/libraries/ESP32-audioI2S-3.0.0/src/Audio.cpp:137
      0x42022b12:  is in Audio::connecttohost(char const*, char const*, char const*) (/home/jake/Arduino/libraries/ESP32-audioI2S-3.0.0/src/Audio.cpp:412).
      0x4200578e:  is in Radio::Radio(RadioConfig*, WiFiManager*, Audio*) (/home/jake/repos/project-super-simple-radio/firmware/LEDStatus.h:132).
      0x4200a30c: Radio::loop() at /home/jake/repos/project-super-simple-radio/firmware/Radio.h:467
      0x4200a34a: Radio::loop() at /home/jake/repos/project-super-simple-radio/firmware/Radio.h:510
      0x4203c4c9: rmt_set_tx_thr_intr_en at /Users/ficeto/Desktop/ESP32/ESP32S2/esp-idf-public/components/hal/esp32s3/include/hal/rmt_ll.h:329

ESP32-audioI2S Examples/Docs/Source (testing 3.0.0) (Installed manually):
  - https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/src/Audio.h
  - https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/src/Audio.cpp
  - https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/examples/Simple_WiFi_Radio/Simple_WiFi_Radio.ino

WiFiManager (2.0.16-rc.2):
  - https://github.com/tzapu/WiFiManager

ArduinoJSON (6.21.2)
  - https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
    
*/
#define FIRMWARE_VERSION "v1.0.0-beta.2"

#include <WiFiManager.h>
#include "Audio.h"
#include "Radio.h"

RadioConfig radio_config;
WiFiManager wifi_manager;
Audio audio;
LEDStatusConfig led_status_config;

Radio radio(&radio_config, &wifi_manager, &audio, &led_status_config);

// Callbacks which need the radio instance
void audio_info(const char *info) {
  // Triggered once a feed is played.
  if (radio.m_debug_mode) {
    Serial.print("info:");
    Serial.println(info);
  }
}

void wifi_manager_setup_callback(WiFiManager *myWiFiManager) {
  radio.m_led_status.set_status(RADIO_STATUS_450_UNABLE_TO_CONNECT_TO_WIFI_WM_ACTIVE);
  if (radio.m_debug_mode) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.print("Created config portal AP ");
    Serial.println(myWiFiManager->getConfigPortalSSID());
  }
}

void setup() {

  // radio.m_debug_mode = true;

  wifi_manager.setAPCallback(wifi_manager_setup_callback);
  radio.init();
}

void loop() {
  radio.loop();
}