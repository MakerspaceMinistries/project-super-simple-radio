# Example Commands #

**Write Firmware**

python serial_programmer.py write_firmware -V v1.0.0-beta.6 -t /dev/ttyACM0

# Installing Firmware #

By default, the ESP32S3s cannot be programmed via the USB port. This needs to be enabled in firmware by writing the firmware over a serial connection.

## Compiling Firmware ##

You only need to do this if you've made changes to the source code. Otherwise, you can find the compiled firmware in this folder.

The README.md in the firmware folder explains how to compile new firmware.

# Workflow #

## Two Step (Prelim to Enable USB, Final over USB to update) ##

**If this is the first you are writing to the ESP32S3, then you MUST write to the serial interface. The USB can only be used to program *after* you have written firmware over the serial interface. The firmware will instruct the ESP32S3 to make the USB port available for writing firmware.**

1. Connect the board using an FTDI adapter (required if this is the first write) or via USB.
2. (Ignore this if writing over USB) Press the Reset->Program button.
3. Open a terminal and navigate to this folder and execute:
    Windows:
        .\write-firmware.bat v1.0.0-beta.2 [port ex: COM1] [folder of desired firmware version ex: v1.0.0]
    Linux:
        /./write-firmware.sh v1.0.0-beta.2 [port ex: /dev/ttyACM0] [folder of desired firmware version ex: v1.0.0]



TO HERE


<!-- 1. Writing the firmware.
    1. Using an adapter, write the firmware to the board before install in a box with esptool.py. This way it will be prepared to receive firmware and serial over the USB port
    2. Board is installed in box.
    3. (Optional) firmware is updated
    4. Python script generates an ID, sends it via serial, prints label to place on box. (Radio is added to database???) -->

## Misc ##

### Figuring out the command Arduino IDE uses to write firmware ###

To see how the Arduino IDE interacts with tools like esptool.py, you can change these settings to have the Arduino IDE output the commands used to Output window.

File -> Preferences -> Check the boxes for *Show diverse output during* compile & upload.

This can help to figure out how to write firmware using esptool.py, by providing a real world example.

You can also see which libraries are used, and where they are pulled from.
