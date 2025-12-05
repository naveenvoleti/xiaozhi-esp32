#EchoEar Meow Companion

## Introduction

<div align="center">
    <a href="https://oshwhub.com/esp-college/echoear"><b> Lichuang Open Source Platform </b></a>
</div>

EchoEar is an intelligent AI development kit equipped with ESP32-S3-WROOM-1 module, 1.85-inch QSPI circular touch screen, dual-microphone array, and supports offline voice wake-up and sound source localization algorithms. For hardware details, please check [Echoear Open Source Project](https://oshwhub.com/esp-college/echoear).

## Configuration and compilation commands

**Configure the compilation target as ESP32S3**

```bash
idf.py set-target esp32s3
```

**Open menuconfig and configure**

```bash
idf.py menuconfig
```

Configure the following options respectively:

### Basic configuration
-`Xiaozhi Assistant` → `Board Type` → Select `EchoEar`

### UI style selection
EchoEar supports a variety of different UI display styles, which can be selected through menuconfig configuration:

-`Xiaozhi Assistant` → `Select display style` → Select display style

#### Optional style

##### Emote animation style -Recommended
-**Configuration Options**: `USE_EMOTE_MESSAGE_STYLE`
-**Feature**: Use custom `EmoteDisplay` emoticon display system
-**Function**: Supports rich expression animation, eye animation, status icon display
-**Applicable**: Intelligent assistant scenarios, providing a more vivid human-computer interaction experience
-**Class**: `emote::EmoteDisplay`

**⚠️Important**: Selecting this style requires additional configuration of custom resource files:
1. `Xiaozhi Assistant` → `Flash Assets` → Select `Flash Custom Assets`
2. `Xiaozhi Assistant` → `Custom Assets File` → Fill in the resource file address:
   ```
https://dl.espressif.com/AE/wn9_nihaoxiaozhi_tts-font_puhui_common_20_4-echoear.bin
   ```

##### Default message style (Enable default message style)
-**Configuration Options**: `USE_DEFAULT_MESSAGE_STYLE` (default)
-**Feature**: Use standard message display interface
-**Function**: Traditional text and icon display interface
-**Applicable**: Standard dialogue scenes
-**Class**: `SpiLcdDisplay`

##### WeChat Message Style (Enable WeChat Message Style)
-**Configuration Options**: `USE_WECHAT_MESSAGE_STYLE`
-**Features**: Imitation WeChat chat interface style
-**Function**: WeChat-like message bubble display
-**Applicable**: Users who like WeChat style
-**Class**: `SpiLcdDisplay`
> **Note**: EchoEar uses 16MB Flash and needs to use special partition table configuration to reasonably allocate storage space to applications, OTA updates, resource files, etc.

Press `S` to save, `Q` to exit.

**Compile**

```bash
idf.py build
```

**Burn**

Connect EchoEar to the computer, **turn on the power**, and run:

```bash
idf.py flash
```