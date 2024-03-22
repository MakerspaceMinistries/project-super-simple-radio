/*

Example serial messages for configuring the radio. It is too large to send all at once, so it is broken into parts.

{
  "remote_cfg_url":"http://config.example.com/api/v1/radios/device_interface/v1.0/",
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
  "stn_1_url":"http://example.com/connect-test.mp3",
  "stn_2_url":"http://example.com/connect-test.mp3"
}

{
  "stn_3_url":"http://example.com/connect-test.mp3",
  "stn_4_url":"http://example.com/connect-test.mp3"
}

Example message for resetting the stored preferences and restarting the ESP.

{"clear_preferences": true}
{"restart_esp": true}

TODO document: reset_wifi, debug_mode, ssid, pass

States: 
  - Green               LED_STATUS_SUCCESS              success
  - Green (blinking)    LED_STATUS_SUCCESS_BLINKING     success (blinking)
  - Blue                LED_STATUS_INFO                 info
  - Blue (blinking)     LED_STATUS_INFO_BLINKING        info (blinking)
  - Yellow              LED_STATUS_WARNING              warning
  - Yellow              LED_STATUS_WARNING_BLINKING     warning (blinking)
  - Red                 LED_STATUS_ERROR                error
  - Red (blinking)      LED_STATUS_ERROR_BLINKING       error (blinking)

*/
#define FIRMWARE_VERSION "v1.0.0-beta.4"

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