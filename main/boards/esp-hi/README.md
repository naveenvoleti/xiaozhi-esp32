#ESP-Hi

## Introduction

<div align="center">
    <a href="https://oshwhub.com/esp-college/esp-hi"><b> Lichuang Open Source Platform </b></a>
    |
    <a href="https://www.bilibili.com/video/BV1BHJtz6E2S"><b> Bilibili </b></a>
</div>

ESP-Hi is an open source ESP Friends based on ultra-low-cost AI dialogue robot based on ESP32C3. ESP-Hi integrates a 0.96-inch color screen for displaying expressions. **The robot dog has implemented dozens of actions**. By fully exploring the peripherals of ESP32-C3, only minimal board-level hardware is needed to achieve voice pickup and sound generation. The software is simultaneously optimized to reduce memory and Flash usage, and **wake word detection**and multiple peripheral drivers are simultaneously implemented under resource constraints. Hardware details, etc. can be found in [Lichuang Open Source Project](https://oshwhub.com/esp-college/esp-hi).

## WebUI
ESP-Hi x Xiaozhi has a built-in WebUI for controlling body movements. Please connect your phone and ESP-Hi to the same Wi-Fi, and access `http://esp-hi.local/` on your phone to use it.

If you want to disable it, please uncheck `ESP_HI_WEB_CONTROL_ENABLED`, that is, uncheck `Component config` → `Servo Dog Configuration` → `Web Control` → `Enable ESP-HI Web Control`.

## Configuration and compilation commands

Since ESP-Hi needs to configure many sdkconfig options, it is recommended to compile using a compilation script.

**Compile**

```bash
python ./scripts/release.py esp-hi
```

If you need to compile manually, please refer to `esp-hi/config.json` to modify the corresponding menuconfig options.

**Burn**

```bash
idf.py flash
```


> [!TIP]
>
> **Servo control will occupy the USB Type-C interface of ESP-Hi**, resulting in the inability to connect to the computer (unable to burn/view running logs). If this happens, please follow the tips below:
>
> **Burn**
>
> 1. Disconnect the power supply of ESP-Hi, leaving only the head and not connected to the body.
> 2. Press and hold the ESP-Hi button and connect to the computer.
>
> At this point, ESP-Hi (ESP32C3) should be in burning mode and you can use the computer burning program. After burning is completed, you may need to replug and unplug the power supply.
>
> **View log**
>
> Please set `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`, i.e. `Component config` → `ESP System Settings` → `Channel for console output` and select `USB Serial/JTAG Controller`. This will also disable the servo control function.