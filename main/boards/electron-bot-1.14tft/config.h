#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>
#include <driver/uart.h>

// --- Audio Configuration (I2S) ---
#define AUDIO_INPUT_SAMPLE_RATE 24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

#define AUDIO_I2S_METHOD_SIMPLEX
#define DISPLAY_2_0TFT
#define HORIZONTAL_SCREEN_ORIENTATION


#ifdef AUDIO_I2S_METHOD_SIMPLEX

#define ENABLE_CAMERA


#define MotorSerial

#define AUDIO_I2S_MIC_GPIO_WS GPIO_NUM_14
#define AUDIO_I2S_MIC_GPIO_SCK GPIO_NUM_13
#define AUDIO_I2S_MIC_GPIO_DIN GPIO_NUM_12
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_21
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_47
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_45

// #define AUDIO_I2S_MIC_GPIO_WS   GPIO_NUM_4
// #define AUDIO_I2S_MIC_GPIO_SCK  GPIO_NUM_5
// #define AUDIO_I2S_MIC_GPIO_DIN  GPIO_NUM_6
// #define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_9
// #define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_10
// #define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_11

#else
#define AUDIO_I2S_MIC_GPIO_WS GPIO_NUM_12
#define AUDIO_I2S_MIC_GPIO_SCK GPIO_NUM_13
#define AUDIO_I2S_MIC_GPIO_DIN GPIO_NUM_14
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_9
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_10
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_11

#endif

// --- Button Configuration ---
#define BUILTIN_LED_GPIO GPIO_NUM_48
#define RGB_LED_GPIO GPIO_NUM_10
#define LAMP_GPIO GPIO_NUM_5
#define BOOT_BUTTON_GPIO GPIO_NUM_0
#define VOLUME_UP_BUTTON_GPIO GPIO_NUM_NC
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_NC
#define TOUCH_BUTTON_GPIO GPIO_NUM_NC
#define ASR_BUTTON_GPIO GPIO_NUM_NC


#ifdef DISPLAY_1_14TFT
// --- Display Configuration (ST7789 1.14 TFT) ---
// Portrait ST7789 configuration 1.14tft
#ifndef HORIZONTAL_SCREEN_ORIENTATION
#define DISPLAY_WIDTH 135
#define DISPLAY_HEIGHT 240
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_OFFSET_X 41
#define DISPLAY_OFFSET_Y 53
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#else
// Horizontal ST7789 configuration 1.14tft
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 135
#define DISPLAY_SWAP_XY true
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y false
#define DISPLAY_OFFSET_X 41
#define DISPLAY_OFFSET_Y 53
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#endif

#else
#ifdef HORIZONTAL_SCREEN_ORIENTATION
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define DISPLAY_SWAP_XY true
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y true
#define DISPLAY_OFFSET_X 0
#define DISPLAY_OFFSET_Y 0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#else
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define DISPLAY_SWAP_XY true
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y true
#define DISPLAY_OFFSET_X 0
#define DISPLAY_OFFSET_Y 0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#endif
#endif


// --- Display Configuration (ST7789 2.0 TFT) ---
// #define DISPLAY_SPI_SCK_PIN GPIO_NUM_5
// #define DISPLAY_SPI_MOSI_PIN GPIO_NUM_4
// #define DISPLAY_DC_PIN GPIO_NUM_7
// #define DISPLAY_SPI_CS_PIN GPIO_NUM_6
// #define DISPLAY_RESET_PIN GPIO_NUM_15 // Add reset pin

#define DISPLAY_SPI_MOSI_PIN GPIO_NUM_39
#define DISPLAY_SPI_SCK_PIN GPIO_NUM_40
#define DISPLAY_SPI_CS_PIN GPIO_NUM_41
#define DISPLAY_DC_PIN GPIO_NUM_42
#define DISPLAY_RESET_PIN GPIO_NUM_2 // Add reset pin
#define DISPLAY_SPI_SCLK_HZ (40 * 1000 * 1000)

#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_1

#define DISPLAY_SPI_MODE 3

// --- Servo Configuration (Electron Bot - assuming these pins are available on DevKit) ---
#define Right_Pitch_Pin GPIO_NUM_NC
#define Right_Roll_Pin GPIO_NUM_NC
#define Left_Pitch_Pin GPIO_NUM_NC
#define Left_Roll_Pin GPIO_NUM_NC
#define Body_Pin GPIO_NUM_NC
#define Head_Pin GPIO_NUM_NC

#define MOTOR_SPEED_MAX 100
#define MOTOR_SPEED_80 80
#define MOTOR_SPEED_60 60
#define MOTOR_SPEED_MIN 0

#ifndef MotorSerial
#define LEDC_M1_CHANNEL_A GPIO_NUM_46
#define LEDC_M1_CHANNEL_B GPIO_NUM_9
#define LEDC_M2_CHANNEL_A GPIO_NUM_16
#define LEDC_M2_CHANNEL_B GPIO_NUM_3
#else
#define UART_ECHO_TXD GPIO_NUM_38
#define UART_ECHO_RXD GPIO_NUM_48
#define UART_ECHO_RTS (-1)
#define UART_ECHO_CTS (-1)

#define ECHO_UART_PORT_NUM      UART_NUM_1
#define ECHO_UART_BAUD_RATE     (115200)
#define BUF_SIZE                (1024)
#endif

// --- Power Management Configuration (DevKit adapted) ---
#define POWER_CHARGE_DETECT_PIN GPIO_NUM_NC
#define POWER_ADC_UNIT ADC_UNIT_1
#define POWER_ADC_CHANNEL ADC_CHANNEL_2


// /* Camera PINs - ORIGINAL PINS KEPT EXACTLY AS-IS */
#ifdef ENABLE_CAMERA
#define EBOT_CAMERA_PWDN      (GPIO_NUM_NC)
#define EBOT_CAMERA_RESET     (GPIO_NUM_NC)
#define EBOT_CAMERA_XCLK      (GPIO_NUM_4)
#define EBOT_CAMERA_PCLK      (GPIO_NUM_5)
#define EBOT_CAMERA_VSYNC     (GPIO_NUM_6)
#define EBOT_CAMERA_HSYNC     (GPIO_NUM_7)
#define EBOT_CAMERA_D0        (GPIO_NUM_15)
#define EBOT_CAMERA_D1        (GPIO_NUM_16)
#define EBOT_CAMERA_D2        (GPIO_NUM_17)
#define EBOT_CAMERA_D3        (GPIO_NUM_18)
#define EBOT_CAMERA_D4        (GPIO_NUM_8)
#define EBOT_CAMERA_D5        (GPIO_NUM_3)
#define EBOT_CAMERA_D6        (GPIO_NUM_46)
#define EBOT_CAMERA_D7        (GPIO_NUM_9)

#define EBOT_CAMERA_XCLK_FREQ (16000000)
#define EBOT_LEDC_TIMER       (LEDC_TIMER_0)
#define EBOT_LEDC_CHANNEL     (LEDC_CHANNEL_0)

#define EBOT_CAMERA_SIOD      (GPIO_NUM_NC)
#define EBOT_CAMERA_SIOC      (GPIO_NUM_NC)
#endif


#define ELECTRON_BOT_VERSION "2.0.4"

#endif // _BOARD_CONFIG_H_









// #ifndef _BOARD_CONFIG_H_
// #define _BOARD_CONFIG_H_

// #include <driver/gpio.h>
// #include <driver/uart.h>

// // --- Audio Configuration (I2S) ---
// #define AUDIO_INPUT_SAMPLE_RATE 24000
// #define AUDIO_OUTPUT_SAMPLE_RATE 24000

// #define AUDIO_I2S_METHOD_SIMPLEX
// #define DISPLAY_2_0TFT
// #define HORIZONTAL_SCREEN_ORIENTATION

// // Uncomment to enable camera
// // #define ENABLE_CAMERA

// #ifdef AUDIO_I2S_METHOD_SIMPLEX
// // Simplex mode: Separate I2S buses for mic and speaker
// // Using safe GPIOs that don't conflict with camera or restrictions
// #define AUDIO_I2S_MIC_GPIO_WS GPIO_NUM_4
// #define AUDIO_I2S_MIC_GPIO_SCK GPIO_NUM_5
// #define AUDIO_I2S_MIC_GPIO_DIN GPIO_NUM_1
// #define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_2
// #define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_42
// #define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_41

// #else
// // Duplex mode: Shared I2S bus for mic and speaker
// #define AUDIO_I2S_MIC_GPIO_WS GPIO_NUM_4
// #define AUDIO_I2S_MIC_GPIO_SCK GPIO_NUM_5
// #define AUDIO_I2S_MIC_GPIO_DIN GPIO_NUM_1
// #define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_2
// #define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_5   // Shared with SCK
// #define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_4   // Shared with WS

// #endif

// // --- Button Configuration ---
// // Avoiding strapping pins (0, 3, 45, 46) except GPIO0 for boot button
// #define BUILTIN_LED_GPIO GPIO_NUM_48
// #define RGB_LED_GPIO GPIO_NUM_38         // Onboard RGB LED
// #define LAMP_GPIO GPIO_NUM_21
// #define BOOT_BUTTON_GPIO GPIO_NUM_0      // Strapping pin but OK for boot button
// #define VOLUME_UP_BUTTON_GPIO GPIO_NUM_NC
// #define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_NC
// #define TOUCH_BUTTON_GPIO GPIO_NUM_14
// #define ASR_BUTTON_GPIO GPIO_NUM_NC

// // --- Display Configuration (ST7789 2.0 TFT) ---
// // Using safe GPIOs avoiding camera, PSRAM (33-37), strapping pins, and USB-JTAG (19,20)
// #define DISPLAY_SPI_SCK_PIN GPIO_NUM_39
// #define DISPLAY_SPI_MOSI_PIN GPIO_NUM_40
// #define DISPLAY_DC_PIN GPIO_NUM_47
// #define DISPLAY_SPI_CS_PIN GPIO_NUM_21
// #define DISPLAY_RESET_PIN GPIO_NUM_20     // GPIO20 USB-JTAG but usable if JTAG disabled
// #define DISPLAY_SPI_SCLK_HZ (40 * 1000 * 1000)

// #ifndef DISPLAY_2_0TFT
// // --- Display Configuration (ST7789 1.14 TFT) ---
// #ifndef HORIZONTAL_SCREEN_ORIENTATION
// #define DISPLAY_WIDTH 135
// #define DISPLAY_HEIGHT 240
// #define DISPLAY_MIRROR_X false
// #define DISPLAY_MIRROR_Y false
// #define DISPLAY_SWAP_XY false
// #define DISPLAY_OFFSET_X 41
// #define DISPLAY_OFFSET_Y 53
// #define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
// #else
// #define DISPLAY_WIDTH 240
// #define DISPLAY_HEIGHT 135
// #define DISPLAY_SWAP_XY true
// #define DISPLAY_MIRROR_X true
// #define DISPLAY_MIRROR_Y false
// #define DISPLAY_OFFSET_X 41
// #define DISPLAY_OFFSET_Y 53
// #define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
// #endif

// #else
// #ifdef HORIZONTAL_SCREEN_ORIENTATION
// #define DISPLAY_WIDTH 320
// #define DISPLAY_HEIGHT 240
// #define DISPLAY_SWAP_XY true
// #define DISPLAY_MIRROR_X false
// #define DISPLAY_MIRROR_Y true
// #define DISPLAY_OFFSET_X 0
// #define DISPLAY_OFFSET_Y 0
// #define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
// #else
// #define DISPLAY_WIDTH 320
// #define DISPLAY_HEIGHT 240
// #define DISPLAY_SWAP_XY true
// #define DISPLAY_MIRROR_X false
// #define DISPLAY_MIRROR_Y true
// #define DISPLAY_OFFSET_X 0
// #define DISPLAY_OFFSET_Y 0
// #define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
// #endif
// #endif

// #define DISPLAY_BACKLIGHT_PIN GPIO_NUM_19   // GPIO19 USB-JTAG but usable if JTAG disabled

// #define DISPLAY_SPI_MODE 3

// // --- Servo Configuration ---
// #define Right_Pitch_Pin GPIO_NUM_NC
// #define Right_Roll_Pin GPIO_NUM_NC
// #define Left_Pitch_Pin GPIO_NUM_NC
// #define Left_Roll_Pin GPIO_NUM_NC
// #define Body_Pin GPIO_NUM_NC
// #define Head_Pin GPIO_NUM_NC

// #define MOTOR_SPEED_MAX 100
// #define MOTOR_SPEED_80 80
// #define MOTOR_SPEED_60 60
// #define MOTOR_SPEED_MIN 0

// #define LEDC_M1_CHANNEL_A GPIO_NUM_43
// #define LEDC_M1_CHANNEL_B GPIO_NUM_44
// #define LEDC_M2_CHANNEL_A GPIO_NUM_NC
// #define LEDC_M2_CHANNEL_B GPIO_NUM_NC

// /* Camera PINs - ORIGINAL PINS KEPT EXACTLY AS-IS */
// #ifdef ENABLE_CAMERA
// #define EBOT_CAMERA_PWDN      (GPIO_NUM_NC)
// #define EBOT_CAMERA_RESET     (GPIO_NUM_NC)
// #define EBOT_CAMERA_XCLK      (GPIO_NUM_15)
// #define EBOT_CAMERA_PCLK      (GPIO_NUM_13)
// #define EBOT_CAMERA_VSYNC     (GPIO_NUM_6)
// #define EBOT_CAMERA_HSYNC     (GPIO_NUM_7)
// #define EBOT_CAMERA_D0        (GPIO_NUM_11)
// #define EBOT_CAMERA_D1        (GPIO_NUM_9)
// #define EBOT_CAMERA_D2        (GPIO_NUM_8)
// #define EBOT_CAMERA_D3        (GPIO_NUM_10)
// #define EBOT_CAMERA_D4        (GPIO_NUM_12)
// #define EBOT_CAMERA_D5        (GPIO_NUM_18)
// #define EBOT_CAMERA_D6        (GPIO_NUM_17)
// #define EBOT_CAMERA_D7        (GPIO_NUM_16)

// #define EBOT_CAMERA_XCLK_FREQ (16000000)
// #define EBOT_LEDC_TIMER       (LEDC_TIMER_0)
// #define EBOT_LEDC_CHANNEL     (LEDC_CHANNEL_0)

// #define EBOT_CAMERA_SIOD      (GPIO_NUM_NC)
// #define EBOT_CAMERA_SIOC      (GPIO_NUM_NC)
// #endif

// // --- Power Management Configuration ---
// #define POWER_CHARGE_DETECT_PIN GPIO_NUM_NC
// #define POWER_ADC_UNIT ADC_UNIT_1
// #define POWER_ADC_CHANNEL ADC_CHANNEL_2

// #define ELECTRON_BOT_VERSION "2.0.4"

// #endif // _BOARD_CONFIG_H_