#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_NC
#define AUDIO_I2S_GPIO_WS   GPIO_NUM_4
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_27
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_34
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_25

#define AUDIO_CODEC_PA_PIN       GPIO_NUM_NC
// I2C pins (not used since no codec, but defined for compatibility)
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_21
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_19
#define AUDIO_CODEC_ES8311_ADDR  0x18  // ES8311 default address

#define BUILTIN_LED_GPIO        GPIO_NUM_NC
// Changed boot button from GPIO_NUM_9 to GPIO_NUM_0 (standard boot button)
#define BOOT_BUTTON_GPIO        GPIO_NUM_0
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_NC
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_NC

#define DISPLAY_SPI_SCK_PIN     GPIO_NUM_42
#define DISPLAY_SPI_MOSI_PIN    GPIO_NUM_41
#define DISPLAY_DC_PIN          GPIO_NUM_1
#define DISPLAY_SPI_CS_PIN      GPIO_NUM_36
#define DISPLAY_RESET_PIN       GPIO_NUM_2  // Add reset pin

// Portrait ST7789 configuration

#define DISPLAY_WIDTH   135
#define DISPLAY_HEIGHT  240
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false

// Landscape ST7789 configuration

// #define DISPLAY_WIDTH   240
// #define DISPLAY_HEIGHT  135
// #define DISPLAY_SWAP_XY true
// #define DISPLAY_MIRROR_X false  // adjust as needed
// #define DISPLAY_MIRROR_Y true

#define DISPLAY_OFFSET_X  52  // (240 - 135 + 1) / 2 = 52 for centered odd width
#define DISPLAY_OFFSET_Y  40  // (320 - 240) / 2 = 40 for centered height

#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_37
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

#endif // _BOARD_CONFIG_H_