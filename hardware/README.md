# Setting up KiCad

## Installing KiCad libraries ##

### Espressif Library ###

Download or clone the repo: https://github.com/espressif/kicad-libraries

Install the symbols, footprints, and 3D models:

1. `Preferences` -> `Manage Symbol Libraries` and add a library and specify the `libraries/Espressif.kicad_sym` in the repo as the Library Path.
2. `Preferences` -> `Manage Footprint Libraries` and add a library and specify the `footprints\Espressif.pretty` in the repo as the Library Path.
3. `Preferences` -> `Configure Paths`
    * Create a new variable:
        * Name: `ESPRESSIF_3DMODELS`
        * Path: `<path to the folder kicad-libraries/3d>`

### MAX98357AETE+T ###

The schematic, footprint, and 3D model can be downloaded here: https://componentsearchengine.com/part-view/MAX98357AETE%2BT/Maxim%20Integrated

### RGB LED ###

The footprint and schematic for the CLMVC-FKC-CGJJM569aBB7a343 wasn't available, however, this LED has the same packing and pinout.

https://componentsearchengine.com/part-view/CLMVC-FKA-CL1D1L71BB7C3C3/Wolfspeed

# Resources #

Many of the circuits required for the radio are also found on the ESP32 S3 Dev Kit. The schematics are helpful for reference.

- Espressif datasheet for the ESP32-S3-WROOM-1/U module https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf
    - Page 25: example schematics
    - Page 12: strapping pins
    - Page 10-11: module pinout
- Espressif ESP32 hardware design guidelines https://www.espressif.com/sites/default/files/documentation/esp32_hardware_design_guidelines_en.pdf
- ESP32 S3 Dev Kit Schematics https://dl.espressif.com/dl/schematics/SCH_ESP32-S3-DevKitC-1_V1.1_20220413.pdf
- ESP32 S3 Dev Kit PCB Layout: https://dl.espressif.com/dl/schematics/PCB_ESP32-S3-DevKitC-1_V1.1_20220429.pdf
- Other Dev Kit resources: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html#related-documents