#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_lcd_types.h> // For LCD_RGB_ELEMENT_ORDER_BGR

// --- Audio Configuration (Minimal, setting conflicting pins to NC) ---
#define AUDIO_INPUT_SAMPLE_RATE 24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// Setting conflicting pins to NC (Not Connected)
#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_NC 
#define AUDIO_I2S_GPIO_WS GPIO_NUM_NC   
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_NC 
#define AUDIO_I2S_GPIO_DIN GPIO_NUM_NC 
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_16 // Assuming this pin is available

#define AUDIO_CODEC_PA_PIN      GPIO_NUM_NC
#define AUDIO_CODEC_I2C_SDA_PIN GPIO_NUM_8
#define AUDIO_CODEC_I2C_SCL_PIN GPIO_NUM_7
#define AUDIO_CODEC_ES8311_ADDR 0x18 

#define BUILTIN_LED_GPIO        GPIO_NUM_22
#define BOOT_BUTTON_GPIO        GPIO_NUM_0

// --- Display Configuration for E32R35T/E32N35T (ST7796, 320x480) ---

// Pins according to E32R35T/E32N35T pin allocation
#define DISPLAY_HOST            SPI3_HOST   // Using SPI3 (VSPI)
#define DISPLAY_SPI_MODE        0           
#define DISPLAY_CS_PIN          GPIO_NUM_15 // TFT_CS -> IO15
#define DISPLAY_MOSI_PIN        GPIO_NUM_13 // TFT_MOSI -> IO13 (SDI/DIN)
#define DISPLAY_MISO_PIN        GPIO_NUM_12 // TFT_MISO -> IO12 (SDO/DOUT)
#define DISPLAY_CLK_PIN         GPIO_NUM_14 // TFT_SCK -> IO14
#define DISPLAY_DC_PIN          GPIO_NUM_2  // TFT_RS -> IO2 (Data/Command)
#define DISPLAY_RST_PIN         GPIO_NUM_NC // TFT_RST -> EN (Hardware Reset)


// Resolution (320x480)
#define DISPLAY_WIDTH           320
#define DISPLAY_HEIGHT          480

// Display Settings
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_RGB_ORDER       LCD_RGB_ELEMENT_ORDER_BGR
#define DISPLAY_INVERT_COLOR    false

#define DISPLAY_OFFSET_X        0
#define DISPLAY_OFFSET_Y        0

// Backlight
#define DISPLAY_BACKLIGHT_PIN   GPIO_NUM_27 // TFT_BL -> IO27
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

#define POWER_CHARGE_DETECT_PIN GPIO_NUM_NC 
#define POWER_ADC_UNIT          ADC_UNIT_1 
#define POWER_ADC_CHANNEL       ADC_CHANNEL_2 


// --- Touch Panel Configuration (XPT2046, SPI) ---
// Pins according to E32R35T/E32N35T pin allocation
#define TOUCH_CS_PIN            GPIO_NUM_33 // TP_CS -> IO33
#define TOUCH_IRQ_PIN           GPIO_NUM_36 // TP_IRQ -> IO36

// A MCP Test: Control a lamp
#define LAMP_GPIO GPIO_NUM_16

#endif // _BOARD_CONFIG_H_