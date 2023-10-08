/*

See the public methods for documentation on how each works

-100     Unset (Idle)
    0- 49 Info
  50- 99 Info (Blinking)
  100-149 Success
  150-199 Success (Blinking)
  200-249 Warning
  250-299 Warning (Blinking)
  300-349 Error
  350-399 Error (Blinking)

Docs on hardware timers
https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/timer.html

*/


#include "esp_system.h"

#define LED_STATUS_BLINKING_THRESHOLD 49
#define LED_STATUS_UNSET -999
#define LED_STATUS_IDLE_STATUS_CODE -100
#define LED_STATUS_LEVEL_000_BLUE_INFO 0
#define LED_STATUS_LEVEL_100_GREEN_SUCCESS 100
#define LED_STATUS_LEVEL_200_YELLOW_WARNING 200
#define LED_STATUS_LEVEL_300_RED_ERROR 300
#define LED_STATUS_MAX_CODE 400

int g_led_status_rgb_pins[3] = { 0, 0, 0 };
bool g_led_status_rgb_is_active[3] = { false, false, false };

void led_status_timer_callback() {
  for (int i = 0; i < 3; i++) {
    if (g_led_status_rgb_is_active[i]) {
      int state = digitalRead(g_led_status_rgb_pins[i]);
      digitalWrite(g_led_status_rgb_pins[i], !state);
    }
  }
}

struct LEDStatusConfig {
  int rgb_pins[3] = { 4, 5, 6 };
  int led_on = LOW;
  int led_off = HIGH;
  int timer_number = 0;
  int timer_divider = 80;
  int timer_alarm_value = 600000;
};

class LEDStatus {
public:
  LEDStatus();
  int get_status();
  void init(LEDStatusConfig *config, bool debug = false);
  void set_status(int status, int force_to_status = LED_STATUS_UNSET);
  void set_debug(bool debug);
  void clear_status(int status);
  void clear_all_statuses();

private:
  LEDStatusConfig *m_config_;
  int m_status_ = LED_STATUS_IDLE_STATUS_CODE;
  int m_major_status_ = -100;
  int m_minor_status_ = 0;
  bool m_debug_ = false;
  hw_timer_t *m_hardware_timer_ = NULL;

  bool is_blinking_status_();
  void write_rgb_(bool rgb[3]);
  void debug_output(const char *message, int value);
  void status_to_rgb_();
  void hardware_timer_enable_();
  void hardware_timer_disable_();
  void set_status_(int status);
};

LEDStatus::LEDStatus(){};

/**
 * Initializes the LEDStatus instance.
 * 
 * This function must be called before using other methods in this class. It sets up the pins for the LEDs and initializes the timer.
 *
 * @param status The LEDStatusConfig object for configuring the pins, timer, and logic.
 * @param debug Boolean. Optional parameter. indicating if debug mode is on. If true, debug messages will be sent to the Serial output.
 */
void LEDStatus::init(LEDStatusConfig *config, bool debug) {
  m_config_ = config;
  m_debug_ = debug;

  for (int i = 0; i < 3; i++) {
    g_led_status_rgb_pins[i] = m_config_->rgb_pins[i];
  }

  pinMode(g_led_status_rgb_pins[0], OUTPUT);
  pinMode(g_led_status_rgb_pins[1], OUTPUT);
  pinMode(g_led_status_rgb_pins[2], OUTPUT);
  bool rgb[3] = { false, false, false };
  write_rgb_(rgb);

  m_hardware_timer_ = timerBegin(m_config_->timer_number, m_config_->timer_divider, true);
  timerAttachInterrupt(m_hardware_timer_, led_status_timer_callback, true);
  timerAlarmWrite(m_hardware_timer_, m_config_->timer_alarm_value, true);
}

/**
 * Returns the current status code.
 * 
 * @return The current status code set in the LEDStatus instance.
 */
int LEDStatus::get_status() {
  return m_status_;
}

/**
 * Sets the status code.
 * 
 * This function will update the LED's color and blink state based on the status code provided. If the current status is more important than the new status, the new status won't overwrite the current status unless forced.
 *
 * @param status The new status code to set.
 * @param force_to_status Optional parameter. If provided and is greater than the current status, this status will be set even if it's considered less important than the current status.
 */
void LEDStatus::set_status(int status, int force_to_status) {

  force_to_status = (force_to_status == LED_STATUS_UNSET) ? status : force_to_status;

  if (status == m_status_) return;

  if (m_status_ <= force_to_status) {
    debug_output("New status: ", status);
    set_status_(status);
    status_to_rgb_();
  } else {
    debug_output("Unable to overwrite current status with: ", status);
  }
}

/**
 * Clears a specific status.
 * 
 * If the current status matches the provided status code, the status will be cleared and set to the unset (idle) state.
 *
 * @param status The status code to clear.
 */
void LEDStatus::clear_status(int status) {
  if (m_status_ == status) {
    debug_output("Clearing status: ", status);
    set_status(LED_STATUS_IDLE_STATUS_CODE, status);
  } else {
    // This needs to be optional or removed as it may be used often "just in case" and will result in flooding the debug output
    // debug_output("Attempting to clear status that is not active: ", status);
  }
}

/**
 * Clears all statuses.
 * 
 * Resets the status code to the unset (idle) state regardless of the current status.
 */
void LEDStatus::clear_all_statuses() {
  debug_output("Resetting status to: ", LED_STATUS_IDLE_STATUS_CODE);
  set_status(LED_STATUS_IDLE_STATUS_CODE, LED_STATUS_MAX_CODE);
}

/**
 * Enables or disables debug mode.
 * 
 * In debug mode, messages are sent to the Serial output to help diagnose issues and monitor the behavior of the LEDStatus instance.
 *
 * @param debug Boolean indicating if debug mode should be enabled (true) or disabled (false).
 */
void LEDStatus::set_debug(bool debug) {
  m_debug_ = debug;
}

void LEDStatus::set_status_(int status) {
  m_status_ = status;
  m_major_status_ = (status / 100) * 100;
  m_minor_status_ = status - m_major_status_;
}

void LEDStatus::write_rgb_(bool rgb[3]) {
  for (int i = 0; i < 3; i++) {
    g_led_status_rgb_is_active[i] = rgb[i];
    int state = (rgb[i]) ? m_config_->led_on : m_config_->led_off;
    digitalWrite(g_led_status_rgb_pins[i], state);
  }
}

void LEDStatus::debug_output(const char *message, int state) {
  if (m_debug_) {
    Serial.print(message);
    Serial.println(state);
  }
}

bool LEDStatus::is_blinking_status_() {
  return (m_minor_status_ > LED_STATUS_BLINKING_THRESHOLD);
}

void LEDStatus::status_to_rgb_() {
  hardware_timer_disable_();

  bool rgb[3] = { false, false, false };

  switch (m_major_status_) {
    case LED_STATUS_IDLE_STATUS_CODE:
      // Do nothing, leaving rgb at it's default value of all LEDs being off.
      break;
    case LED_STATUS_LEVEL_000_BLUE_INFO:
      rgb[2] = true;
      break;
    case LED_STATUS_LEVEL_100_GREEN_SUCCESS:
      rgb[1] = true;
      break;
    case LED_STATUS_LEVEL_200_YELLOW_WARNING:
      rgb[0] = true;
      rgb[1] = true;
      break;
    case LED_STATUS_LEVEL_300_RED_ERROR:
      rgb[0] = true;
      break;
  }

  write_rgb_(rgb);

  if (is_blinking_status_()) {
    hardware_timer_enable_();
  }
}

void LEDStatus::hardware_timer_enable_() {
  timerAlarmEnable(m_hardware_timer_);
}

void LEDStatus::hardware_timer_disable_() {
  timerAlarmDisable(m_hardware_timer_);
}
