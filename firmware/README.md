# Resources #

Using the on board USB for programming is documented here: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/usb_cdc.html

## Programming The ESP32 S3 ##

### TODO ###

The ESP32 S3 can be programmed over the serial port (requiring a USB->Serial device, ex: HW-417) or with the built in USB-CDC.

For this project, the requirements are:

- Serial Connection
  - Programming Firmware
  - Serial Communication (for debugging)
- USB-CDC
  - Programming Firmware after 

A configuration needs to be found so that the above is possible without having the reconfigure the ESP32 S3 (this way units aren't shipped in a state where they need to be reprogrammed with a serial connection in order to make updating the firmware via USB-CDC possible.)

Below is the current state of figuring this out.

### Programming with the Arduino IDE via serial port ###
1. Set the board settings (Tools -> )
    - USB CDC on Boot: Disabled (?)
    - USB DFU on Boot: Disabled (?)
    - USB Firmware MSC on Boot: Disabled
    - USB Mode: USB OTG
    - Upload Mode UART
2. Boot the ESP32 S3 into download mode (hold the boot button while pressing the reset button)
3. Reset the ESP32 S3 (this time pressing only the reset button), putting it back into normal mode (the S3 will not reset itself automatically as other dev boards do.)

### Programming with the Arduino IDE via built in USB ###

1. Set the board settings (Tools -> )
    - USB CDC on Boot: Enabled (?)
    - USB DFU on Boot: Disabled (?)
    - USB Firmware MSC on Boot: Disabled
    - USB Mode: USB OTG
    - Upload Mode UART
2. (? - is this needed for USB Mode?) Boot the ESP32 S3 into download mode (hold the boot button while pressing the reset button)
3. (? - is this needed for USB Mode?) Reset the ESP32 S3 (this time pressing only the reset button), putting it back into normal mode (the S3 will not reset itself automatically as other dev boards do.)
