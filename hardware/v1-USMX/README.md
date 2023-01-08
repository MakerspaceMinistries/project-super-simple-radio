# Parts & Datasheets #

## ESP32-S3-WROOM-1U-N16R8 ##

Purchase: https://www.mouser.com/ProductDetail/356-ESP32S3WM1UN16R8

### Datasheets And Other Resources ###

Many of the circuits required for the radio are also found on the ESP32 S3 Dev Kits & ESP32 S3 Korvo Dev Board, . The schematics are helpful for reference.

- Espressif datasheet for the ESP32-S3-WROOM-1/U module https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf
    - Page 25: example schematics
    - Page 12: strapping pins
    - Page 10-11: module pinout
- Espressif ESP32 hardware design guidelines https://www.espressif.com/sites/default/files/documentation/esp32_hardware_design_guidelines_en.pdf
- ESP32 S3 Dev Kit Schematics https://dl.espressif.com/dl/schematics/SCH_ESP32-S3-DevKitC-1_V1.1_20220413.pdf
- ESP32 S3 Dev Kit PCB Layout: https://dl.espressif.com/dl/schematics/PCB_ESP32-S3-DevKitC-1_V1.1_20220429.pdf
- Other Dev Kit resources: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html#related-documents
- ESP32 S3 Korvo (an audio dev board from Espressif): https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/user-guide-esp32-s3-korvo-2.html

### Footprints/Schematics ###

Download or clone the Espressif Library repo: https://github.com/espressif/kicad-libraries

Install the symbols, footprints, and 3D models:

1. `Preferences` -> `Manage Symbol Libraries` and add a library and specify the `libraries/Espressif.kicad_sym` in the repo as the Library Path.
2. `Preferences` -> `Manage Footprint Libraries` and add a library and specify the `footprints\Espressif.pretty` in the repo as the Library Path.
3. `Preferences` -> `Configure Paths`
    * Create a new variable:
        * Name: `ESPRESSIF_3DMODELS`
        * Path: `<path to the folder kicad-libraries/3d>`

## MAX98357AETE+T - DAC & Amplifier ##

Datasheet: https://www.mouser.com/datasheet/2/256/MAX98357A-MAX98357B-271244.pdf  
Purchase: https://www.mouser.com/ProductDetail/Maxim-Integrated/MAX98357AETE%2bT?qs=AAveGqk956HhNpoJjF5x2g%3D%3D
Footprint/Schematic: https://componentsearchengine.com/part-view/MAX98357AETE%2BT/Maxim%20Integrated 

## AZ1084CD-3.3TRG1 - Voltage Regulator (3.3v) ##

Datasheet: https://www.diodes.com/assets/Datasheets/AZ1084C.pdf  
Purchase: https://www.mouser.com/ProductDetail/621-AZ1084CD-3.3TRG1

This footprint and schematic are for the same line as the AZ1084CD-3.3TRG1

https://componentsearchengine.com/part-view/AZ1084CD-ADJTRG1/Diodes%20Inc.

## P110KH1-0F15BR10Kb - Volume Potentiomoter ##

Datasheet: https://www.mouser.com/datasheet/2/414/TTRB_S_A0010315417_1-2565498.pdf  
Purchase: https://www.mouser.com/ProductDetail/BI-Technologies-TT-Electronics/P110KH1-0F15BR10K?qs=%252BUYXD5bnyXryQEHfrY0%252Baw%3D%3D
Footprint/Schematic: https://componentsearchengine.com/part-view/P110KH1-0F15BR10K/BI%20Technologies

## PTA3043-2015DPB104 - Station Potentiometer ##

Datasheet: https://www.mouser.com/datasheet/2/54/pta-778345.pdf  
Purchase:  https://www.mouser.com/ProductDetail/Bourns/PTA3043-2015DPB104?qs=zRGnDeWDVj0fVt3rAxibYA%3D%3D
Footprint/Schematic: https://componentsearchengine.com/part-view/PTA3043-2015DPB104/Bourns

## CLMVC-FKC-CGJJM569aBB7a343 - RGB LED ##

Datasheet: https://www.mouser.com/datasheet/2/723/1273_CLMVC_FKA-2326704.pdf  
Purchase: https://www.mouser.com/ProductDetail/Cree-LED/CLMVC-FKC-CGJJM569aBB7a343?qs=1mbolxNpo8epafm4p5CBBQ%3D%3D

The footprint and schematic for the CLMVC-FKC-CGJJM569aBB7a343 wasn't available, however, this LED has the same packing and pinout.

https://componentsearchengine.com/part-view/CLMVC-FKA-CL1D1L71BB7C3C3/Wolfspeed

## GG060305100N2P - ESD Suppressor ##

***IMPORTANT*** & ***TODO***  
*The selection of this ESD suppressor may be wrong. The ESP32 S3 Dev Kit and the ESP32 S3 Korvo both use the LESD5D5.0CT1G, which is no longer available. The GG060305100N2P appears to have similar specs, but this should be verified by someone with who is more knowledgeable*

Datasheet: https://www.mouser.com/datasheet/2/40/AVX_GiGuard-2401264.pdf  
Purchase: https://www.mouser.com/ProductDetail/KYOCERA-AVX/GG060305100N2P?qs=QNEnbhJQKvZ38qL0Ea5hYg%3D%3D  
Package/Case: 0603 (1610 metric)
Footprint/Schematic: https://componentsearchengine.com/part-view/GG060305100N2P/Kyocera%20AVX

## Resistors ##

Package/case: 1206/3216 (in/mm)
Footprint: Built-in 1206 footprint

270 ohm
- CR1206-FX-2700ELF
- https://www.mouser.com/ProductDetail/Bourns/CR1206-FX-2700ELF?qs=UUWDWenVDLhTOwaXSP1D9Q%3D%3D

10k ohm
- CR1206-FX-1002ELF
- https://www.mouser.com/ProductDetail/Bourns/CR1206-FX-1002ELF?qs=3wWX5hyYeeQyTehaX7pP%252BA%3D%3D

220k ohm
- CR1206-FX-2203ELF
- https://www.mouser.com/ProductDetail/Bourns/CR1206-FX-2203ELF?qs=UUWDWenVDLhp0KvMMxHtXw%3D%3D

## 10uF Ceramic Capacitor ##

Package/case: 0805/2012 (in/mm)
Footprint: Built-in 0805 footprint

10uF 16v
- Model: CL21B106KAYQNNE
- Purchase: https://www.mouser.com/ProductDetail/187-CL21A106KOQNNNG

0.1uF 25v
- Model: CL21B104KACNNNC
- Purchase: https://www.mouser.com/ProductDetail/Samsung-Electro-Mechanics/CL21B104KACNNNC?qs=yOVawPpwOwnhB47kNbieFw%3D%3D

## Tactile Switches - Reset and Reset & Program ##

Datasheet: https://www.mouser.com/datasheet/2/240/pts526-3050698.pdf 
Purchase: https://www.mouser.com/ProductDetail/CK/PTS526-SM08-SMTR2-LFS?qs=UXgszm6BlbFgRZzlfGAMFg%3D%3D
Footprint/Shematic: https://componentsearchengine.com/part-view/PTS526%20SM08%20SMTR2%20LFS/C%20%26%20K%20COMPONENTS

## USB Micro ##

Datasheet: https://cdn.amphenol-cs.com/media/wysiwyg/files/documentation/datasheet/inputoutput/io_usb_micro.pdf
Drawing: https://www.amphenol-cs.com/media/wysiwyg/files/drawing/10118192.pdf  
Purchase: https://www.mouser.com/ProductDetail/Amphenol-FCI/10118192-0002LF?qs=KVgMXE4aH4nIkuWlMxQzog%3D%3D
Footprint/Schematic: https://componentsearchengine.com/part-view/10118192-0002LF/Amphenol%20Communication%20Solutions 

# Other Hardware Options #

These are options that were not used, but may be useful for future designs.

## Speakers ##
https://www.aliexpress.us/item/2251832407677186.html?spm=a2g0o.order_list.order_list_main.99.21ef1802j2SpLD&gatewayAdapt=glo2usa4itemAdapt&_randl_shipto=US

## 90 Tactile Switch - Reset ##
https://www.mouser.com/ProductDetail/E-Switch/TL3336AF160Q?qs=iLbezkQI%252Bshf0NMs3%252BGhxA%3D%3D