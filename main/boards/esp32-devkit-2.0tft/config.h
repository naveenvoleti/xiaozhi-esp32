// fileName: config.h

#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_NC
#define AUDIO_I2S_GPIO_WS   GPIO_NUM_26
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

#define DISPLAY_SPI_SCK_PIN     GPIO_NUM_18
#define DISPLAY_SPI_MOSI_PIN    GPIO_NUM_23
#define DISPLAY_DC_PIN          GPIO_NUM_2
#define DISPLAY_SPI_CS_PIN      GPIO_NUM_15
#define DISPLAY_RESET_PIN       GPIO_NUM_4  // Add reset pin

// Portrait ST7789 configuration for 240x320 display
// NOTE: ST7789 physical panel is often 240x320, the 1.14" display was a cropped version.

// #define DISPLAY_WIDTH   240 // <-- CHANGED
// #define DISPLAY_HEIGHT  320 // <-- CHANGED
// #define DISPLAY_MIRROR_X false
// #define DISPLAY_MIRROR_Y false
// #define DISPLAY_SWAP_XY false

// Landscape ST7789 configuration (If preferred)
#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240
#define DISPLAY_SWAP_XY true
#define DISPLAY_MIRROR_X false  // adjust as needed
#define DISPLAY_MIRROR_Y true

// Set offsets to 0, 0 for full 240x320 display, assuming the driver supports it directly.
// If the panel is 240x320 (like the ST7789) then no offset is needed.
#define DISPLAY_OFFSET_X  0  // <-- CHANGED (240-240)/2 = 0
#define DISPLAY_OFFSET_Y  0  // <-- CHANGED (320-320)/2 = 0

#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_22
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

#endif // _BOARD_CONFIG_H_