# Project concept

## Features & Value proposition
- 4 relays channels
- 4 FET channels
- 1 USB switch
- 1 SD card switch
- Control can be done via Serial (USB) or Bluetooth
- Ability to configure PWM-like behavior for each channel with a single command
- Upgradable firmware over Serial (USB) with rollback capability
- Powered via 12Vdc external power source

## Technologies
- Zephyr RTOS as platform
- C++
- Protobuf specifies the communication protocols between host and target to allow for:
    - Controlling relay channels
    - Controlling FET channels
    - Controlling USB Switch
    - Controlling SD Switch
    - Sending firmware packages for firmware upgrade
- mcuboot as bootloader
- ESP32C3 as microcontroller (also Bluetooth chip)
    - Alternatives (?)
- Kicad used for hardware development

## Architecture

## Breakdown into smaller steps

This section breaks down the bigger picture project into smaller independent project that can be implemented and tested by their own.

### Project: python script
- A python script shall be developed to allow for interacting (testing) the embedded application
- This script should allow to interact with all of the features of the device via CLI

### Project: mcuboot
- Start by making mcuboot Zephyr example work on target board
- Develop protobuf for communication between host and target to exchange firmware packages, for firmware upgrade
- Implement python script to parse input binary (built firmware) and send it to target via serial UART using protobuf specification
- Implement test setup where there are 2 built binaries that blink different LEDs and switch between firmwares using python script

### Project: bluetooth
- Choose one of the Zephyr Bluetooth examples to run on target board
- Device will be a Bluetooth scannable and connectable advertiser
- Project a Bluetooth profile

### Project: development of custom hardware

#### Kicad circuit and board design
- Study Kicad to begin with
#### Zephyr custom board definition
- Develop simple blink LED for custom board

### Integrate smaller projects into one single project