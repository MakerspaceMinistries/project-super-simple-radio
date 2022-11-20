# Firmware Updates #

## Using the on board USB for programming ##

This is documented here: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/usb_cdc.html

## Programming with the Arduino IDE via built in USB ##

1. Set the board settings (Tools -> )
    - USB CDC on Boot: Enabled
    - USB DFU on Boot: Disabled (?)
    - USB Firmware MSC on Boot: Disabled
    - USB Mode: USB OTG
    - Upload Mode UART
2. Boot the ESP32 S3 into download mode (hold the boot button while pressing the reset button)
3. Reset the ESP32 S3 (this time pressing only the reset button), putting it back into normal mode (the S3 will not reset itself automatically as other boards do.)

