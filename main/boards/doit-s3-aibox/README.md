#sibozhilian AI Companion Box

# Features
*Use PDM microphone
*Use common anode LED

## Button configuration
*BUTTON3: short press-interrupt/wake up
*BUTTON1: volume +
*BUTTON2: volume -

## Compile configuration command

**Configure the compilation target as ESP32S3:**

```bash
idf.py set-target esp32s3
```

**Open menuconfig:**

```bash
idf.py menuconfig
```

**Select Board:**

```
Xiaozhi Assistant -> Board Type -> Sibo Zhilian AI Companion Box
```

**Modify psram configuration:**

```
Component config -> ESP PSRAM -> SPI RAM config -> Mode (QUAD/OCT) -> Octal Mode PSRAM
```

**Compile:**

```bash
idf.py build
```