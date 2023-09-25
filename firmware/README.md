# Resources #

Using the on board USB for programming is documented here: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/usb_cdc.html

## Programming The ESP32 S3 ##

This is a two step process. First, the firmware must be written with a USB to Serial programmer (ex: HW-417) with certain options so that the USB CDC is enabled (the onboard USB to serial). From then, the board can be programmed using the USB port.

https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/cdc_dfu_flash.html#usb-cdc

1. Press the program reset button on the board to boot to programming mode.
2. Use a programmer to upload the firmware using the following settings in Tools:
    - USB CDC on Boot: Enabled
    - USB DFU on Boot: Disabled
    - PSRAM: OPI PSRAM
    - USB Firmware MSC on Boot: Disabled
    - Upload Mode UART
    - USB Mode: Hardware CDC and JTAG
    
Now, using the same settings above, the USB port can be used to program as well as for serial. Pressing program reset is no longer needed.

# LED Statuses #

Statuses are hierarchical - with errors overwriting all other status, and warnings overwriting info and success statuses.

Statuses have different meanings depending on whether they occur during the Setup or Loop phases.

Setup runs once when the radio is powered on. After finishing the initial setup of the device, the radio enters the Loop phase, where it remains until it is powered off.

## Error (Red) ##

### Setup ###

- Error: Unable to connect to WiFi, WiFiManager Active
  - Set by: When it cannot connect to WiFi
  - Cleared By: When WiFi connects

- Error (Blinking): After WiFiManager, unable to connect to WiFi
  - Set by: wifiManager.autoConnect("Radio Setup") returns false upon failure, triggering this error.
  - Cleared By: After 10s (10000ms), the Esp will restart, which will reset the status.

### Loop ###

- Error: WiFi connection lost
  - Set by: Radio::checkWiFiDisconnect() checks WiFi.status() and compares to WL_CONNECTED periodically. If connection is lost, the error is set and the WiFi reconnection process is started.
  - Cleared By: If WiFi.status() is WL_CONNECTED, the error is cleared.

- Error (Blinking): Stream connection lost, after x seconds of retrying
  - Set by: Radio::checkStreamStatus(), by looking at audio->getAudioCurrentTime() periodically and checking if it has advanced compared to what it was at last check.
  - Clear: Radio::checkStreamStatus(), if the getAudioCurrentTime() is advancing, then this is cleared.

## Warning (Yellow) ##

### Setup ###

- Warning:
  - Set by:
  - Cleared By: 

- Warning (Blinking): Unable to connect to remote settings server, using previous settings.
  - Set by: getConfigFromRemote() 
  - Cleared By: getConfigFromRemote(), after 10s (10000ms).

### Loop ###

- Warning: 
  - Set by: 
  - Cleared By: 

- Warning (Blinking): Reconnecting after being previously connected. This status lasts for x seconds, and then it becomes an error.
  - Set by: Radio::checkStreamStatus(), by looking at audio->getAudioCurrentTime() periodically and checking if it has advanced compared to what it was at last check.
  - Cleared by: Radio::checkStreamStatus(), if the getAudioCurrentTime() is advancing, then this is cleared.

## Success (Green) ##

### Setup ###

- Success:
  - Set by: 
  - Cleared By: 

- Success (Blinking): 
  - Set by: 
  - Cleared By: 
  
### Loop ###

- Success: Stream is playing.
  - Set by: 
  - Cleared By: 

- Success (Blinking): 
  - Set by: 
  - Clear: 

## Info (Blue) ##

### Setup ###

- Info: Fading from green to blue happens when the radio turns on, and then the status remains as info until setup is complete.
  - Set by: Radio::init() sets the status to Info while the device enters setup().
  - Cleared By: When the device exits setup(), the Info status is reset.

- Info (Blinking): 
  - Set by: 
  - Cleared By: 

### Loop ###

- Info: 
  - Set by: 
  - Cleared By: 

- Info (Blinking): Connecting to the streaming server.
  - Set by: 
  - Cleared by: 