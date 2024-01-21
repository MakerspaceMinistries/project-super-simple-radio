# Compiling Firmware #

The ESP32S3 can be programmed through both the serial interface as well as the USB interface - if the USB interface has been enabled by first writing the proper firmware over the serial port. 

## 1. Board & Libraries ##

Install these versions of the board and libraries. Some of these may need to installed manually. 

  - Board: esp32 2.0.9
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

  - ESP32-audioI2S: 3.0.0 
      Examples/Docs/Source (Installed manually):
      - https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/src/Audio.h
      - https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/src/Audio.cpp
      - https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/examples/Simple_WiFi_Radio/Simple_WiFi_Radio.ino

  - WiFiManager: 2.0.16-rc.2
    - https://github.com/tzapu/WiFiManager

ArduinoJSON: 6.21.2
  - https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
  - 
## 2. Setting up Arduino's Tools ## 

1. Setup Tools menu with these options:
    - USB CDC on Boot: Enabled
    - USB DFU on Boot: Disabled
    - USB Firmware MSC on Boot: Disabled
    - PSRAM: OPI PSRAM
    - Upload Mode UART
    - USB Mode: Hardware CDC and JTAG

2. Select the ESP32S3 Dev Module as the board (Select the port as well, if you are writing to a device)

## 3. Compiling ##

You can compile and either write directly to a radio, or generate a firmware file.

### 3a. Compiling & Writing to Device ###

If this is the first time you are writing firmware, you will need to use an FTDI adapter to write over serial. Once that has been done, you will be able to write using the USB port.

1. (If writing over USB, you can skip this) Press the program reset button on the board to boot to programming mode.
2. Click the upload button or ctrl+u. 

### 3b. Compiling to a File ###

1. Go to Sketch->Export Compiled Binary

This will create a set of files in the same folder as the firmware, inside a folder/subfolder:

./build/esp32.esp32.esp32s3/

There you will find a few files.

# Helpful Docs #

Using the on board USB for programming is documented here:
  - https://docs.espressif.com/projects/arduino-esp32/en/latest/api/usb_cdc.html
  - https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/cdc_dfu_flash.html#usb-cdc

