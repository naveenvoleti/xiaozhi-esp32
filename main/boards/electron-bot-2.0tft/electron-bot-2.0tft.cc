#include <driver/i2c_master.h>
#include <driver/spi_common.h>
#include "electron_emoji_display.h"
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_log.h>
#include <wifi_station.h>

#include "application.h"
#include "codecs/no_audio_codec.h"
#include "button.h"
#include "config.h"
// #include "devkit_lcd_display.h" // Using the DevKit_LcdDisplay class
#include "driver/spi_master.h"
#include "power_manager.h"
#include "system_reset.h"
#include "wifi_board.h"
#include "simple_pwm_backlight.h" 

#define TAG "ELECTRON_BOT_2_0TFT"

// Controller initialization function declaration from electron_bot_controller.cc
void InitializeElectronBotController();

class ELECTRON_BOT_2_0TFT : public WifiBoard {
private:
    // DevKit_LcdDisplay* display_;
    ElectronEmojiDisplay* display_;
    PowerManager* power_manager_;
    Button boot_button_;

    void InitializePowerManager() {
        // Initialize PowerManager using the configuration pins.
        power_manager_ = new PowerManager(POWER_CHARGE_DETECT_PIN);
    }

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_SPI_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_SPI_SCK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        // Max transfer size updated for 240x320 display
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        buscfg.flags = SPICOMMON_BUSFLAG_MASTER;
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting &&
                !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });
    }

    void InitializeController() { 
        // This function initializes the Otto movement logic and MCP tools
        InitializeElectronBotController(); 
    }

    void InitializeSt7789Display() {
        ESP_LOGI(TAG, "Init ST7789 display");

        ESP_LOGI(TAG, "Install panel IO");
        esp_lcd_panel_io_handle_t io_handle = NULL;
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_SPI_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = 3;
        io_config.pclk_hz = 27 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &io_handle));

        ESP_LOGI(TAG, "Install ST7789 panel driver");
        esp_lcd_panel_handle_t panel_handle = NULL;
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RESET_PIN;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel = 16;

        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
        
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
        
        // Configuration for 1.14" display orientation (matching the config.h defines)
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, DISPLAY_SWAP_XY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

        // Create the display object
        // display_ = new DevKit_LcdDisplay(io_handle, panel_handle, DISPLAY_WIDTH, DISPLAY_HEIGHT,
        //                                     DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X,
        //                                     DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
        display_ = new ElectronEmojiDisplay(io_handle, panel_handle, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                            DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X,
                                            DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
        // display_->SetupHighTempWarningPopup(); // Setup LVGL elements
    }

public:
    ELECTRON_BOT_2_0TFT() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeSpi();
        InitializeSt7789Display();
        InitializeButtons();
        InitializePowerManager();
        InitializeController(); // Initialize servo control (Otto)

        if (DISPLAY_BACKLIGHT_PIN != GPIO_NUM_NC) {
            GetBacklight()->RestoreBrightness();
        }
    }

    virtual AudioCodec* GetAudioCodec() override {
        // Using NoAudioCodecSimplex for boards without a dedicated codec
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE, 
            AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, 
            AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DIN);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override { return display_; }

    virtual Backlight* GetBacklight() override {
        // static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        static SimplePwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }

    virtual bool GetBatteryLevel(int& level, bool& charging, bool& discharging) override {
        charging = power_manager_->IsCharging();
        discharging = power_manager_->IsDischarging();
        level = power_manager_->GetBatteryLevel();
        return true;
    }
    
    virtual bool GetTemperature(float& esp32temp) override {
        esp32temp = 0.0f;
        return false;
    }
};

DECLARE_BOARD(ELECTRON_BOT_2_0TFT);