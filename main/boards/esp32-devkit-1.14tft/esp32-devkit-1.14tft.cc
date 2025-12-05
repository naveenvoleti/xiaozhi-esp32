#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "devkit_lcd_display.h"
#include "system_reset.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "power_save_timer.h"
#include "led/single_led.h"
#include "assets/lang_config.h"

#include <esp_log.h>
#include <esp_lcd_panel_vendor.h>
#include <wifi_station.h>

#define TAG "ESP32_DEVKIT_1_14TFT"

class ESP32_DEVKIT_1_14TFT : public WifiBoard {
private:
    Button boot_button_;
    DevKit_LcdDisplay* display_;
    PowerSaveTimer* power_save_timer_;
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    i2c_master_bus_handle_t i2c_bus_handle_ = nullptr;

    void InitializePowerSaveTimer() {
        power_save_timer_ = new PowerSaveTimer(-1, 60, 300);
        power_save_timer_->OnEnterSleepMode([this]() {
            GetDisplay()->SetPowerSaveMode(true);
            GetBacklight()->SetBrightness(1);
        });
        power_save_timer_->OnExitSleepMode([this]() {
            GetDisplay()->SetPowerSaveMode(false);
            GetBacklight()->RestoreBrightness();
        });
        power_save_timer_->SetEnabled(true);
    }

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_SPI_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_SPI_SCK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        buscfg.flags = SPICOMMON_BUSFLAG_MASTER;
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            power_save_timer_->WakeUp();
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });

        boot_button_.OnLongPress([this]() {
            power_save_timer_->WakeUp();
            auto& app = Application::GetInstance();
            app.SetDeviceState(kDeviceStateWifiConfiguring);
            ResetWifiConfiguration();
        });
    }

    void InitializeSt7789Display() {
        ESP_LOGD(TAG, "Install panel IO");
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_SPI_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = 3;  // Changed to mode 3 (matches Adafruit default)
        io_config.pclk_hz = 27 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        io_config.flags.lsb_first = 0;
        io_config.flags.dc_low_on_data = 0;
        io_config.flags.octal_mode = 0;
        io_config.flags.sio_mode = 0;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io_));

        ESP_LOGD(TAG, "Install LCD driver");
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RESET_PIN;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel = 16;
        panel_config.flags.reset_active_high = 0;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io_, &panel_config, &panel_));
        
        ESP_LOGI(TAG, "Reset panel");
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_));
        vTaskDelay(pdMS_TO_TICKS(150));
        
        ESP_LOGI(TAG, "Initialize panel");
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_));
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Configure for 1.14" display orientation (portrait)
        ESP_LOGI(TAG, "Configure panel orientation");
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_, DISPLAY_SWAP_XY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_, true));
        vTaskDelay(pdMS_TO_TICKS(10));
        
        ESP_LOGI(TAG, "Turn on display");
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_, true));
        vTaskDelay(pdMS_TO_TICKS(10));
        
        ESP_LOGI(TAG, "Create display object");
        display_ = new DevKit_LcdDisplay(panel_io_, panel_, DISPLAY_WIDTH, DISPLAY_HEIGHT, 
            DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, 
            DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
        display_->SetupHighTempWarningPopup();
        
        ESP_LOGI(TAG, "Display initialization complete");
    }

public:
    ESP32_DEVKIT_1_14TFT() :
        boot_button_(BOOT_BUTTON_GPIO) {
        InitializePowerSaveTimer();
        InitializeSpi();
        InitializeButtons();
        InitializeSt7789Display();  
        GetBacklight()->RestoreBrightness();
    }

    virtual AudioCodec* GetAudioCodec() override {
        // Using NoAudioCodecSimplex for boards without audio codec hardware
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,   // Input sample rate
            AUDIO_OUTPUT_SAMPLE_RATE,  // Output sample rate
            AUDIO_I2S_GPIO_BCLK,       // Speaker BCLK pin
            AUDIO_I2S_GPIO_WS,         // Speaker WS (LRCLK) pin
            AUDIO_I2S_GPIO_DOUT,       // Speaker DOUT pin
            AUDIO_I2S_GPIO_BCLK,       // Mic BCLK pin (reuse same)
            AUDIO_I2S_GPIO_WS,         // Mic WS pin (reuse same)
            AUDIO_I2S_GPIO_DIN         // Mic DIN pin
        );
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }
    
    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }

    virtual bool GetBatteryLevel(int& level, bool& charging, bool& discharging) override {
        // No battery management for dev board
        charging = false;
        discharging = false;
        level = 100;
        return false;
    }

    virtual bool GetTemperature(float& esp32temp) override {
        // No temperature sensor for dev board
        esp32temp = 0.0f;
        return false;
    }

    virtual void SetPowerSaveMode(bool enabled) override {
        if (!enabled) {
            power_save_timer_->WakeUp();
        }
        WifiBoard::SetPowerSaveMode(enabled);
    }
};

DECLARE_BOARD(ESP32_DEVKIT_1_14TFT);