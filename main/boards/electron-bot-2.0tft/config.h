#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// --- Audio Configuration (I2S) ---
#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_NC
#define AUDIO_I2S_GPIO_WS   GPIO_NUM_26
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_27
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_34
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_25

#define AUDIO_CODEC_PA_PIN       GPIO_NUM_NC
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_21
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_19
#define AUDIO_CODEC_ES8311_ADDR  0x18

// --- Button Configuration ---
#define BUILTIN_LED_GPIO        GPIO_NUM_NC
#define BOOT_BUTTON_GPIO        GPIO_NUM_0
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_NC
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_NC

// --- ST7789 1.14" Display Configuration (on ESP32 DevKit V1) ---
#define DISPLAY_SPI_SCK_PIN     GPIO_NUM_18
#define DISPLAY_SPI_MOSI_PIN    GPIO_NUM_23
#define DISPLAY_DC_PIN          GPIO_NUM_2
#define DISPLAY_SPI_CS_PIN      GPIO_NUM_15
#define DISPLAY_RESET_PIN       GPIO_NUM_4

// Portrait ST7789 configuration for 240x320 display
// NOTE: ST7789 physical panel is often 240x320, the 1.14" display was a cropped version.

// #define DISPLAY_WIDTH   240
// #define DISPLAY_HEIGHT  320
// #define DISPLAY_SWAP_XY false
// #define DISPLAY_MIRROR_X false
// #define DISPLAY_MIRROR_Y false
// #define DISPLAY_OFFSET_X 0
// #define DISPLAY_OFFSET_Y 0

// Landscape ST7789 configuration for 320x240 display
#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240
#define DISPLAY_SWAP_XY true
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y true
#define DISPLAY_OFFSET_X 0
#define DISPLAY_OFFSET_Y 0

#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_22
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT true

// --- Servo Configuration (Electron Bot - assuming these pins are available on DevKit) ---
#define Right_Pitch_Pin  GPIO_NUM_5
#define Right_Roll_Pin   GPIO_NUM_6
#define Left_Pitch_Pin   GPIO_NUM_16
#define Left_Roll_Pin    GPIO_NUM_17
#define Body_Pin         GPIO_NUM_13
#define Head_Pin         GPIO_NUM_12

// --- Power Management Configuration (DevKit adapted) ---
#define POWER_CHARGE_DETECT_PIN GPIO_NUM_NC 
#define POWER_ADC_UNIT          ADC_UNIT_1 
#define POWER_ADC_CHANNEL       ADC_CHANNEL_2 

#define ELECTRON_BOT_VERSION "2.0.4"

#endif // _BOARD_CONFIG_H_