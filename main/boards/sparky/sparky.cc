#include <driver/i2c_master.h>
#include "dual_network_board.h"
#include <driver/spi_common.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_log.h>
#include <wifi_station.h>
#include "sparky_emoji_display.h"
#include "application.h"
#include "codecs/no_audio_codec.h"
#include "button.h"
#include "config.h"
#include "led/single_led.h"
#include "lamp_controller.h"
#include "driver/spi_master.h"
#include "power_manager.h"
#include "system_reset.h"
#include "wifi_board.h"
#include "mcp_server.h"
#include "display/emote_display.h"
#include "display/lcd_display.h"

#define TAG "SPARKY"

// Controller initialization function declaration from sparky_controller.cc
void InitializeSparkyController();

class SPARKY : public WifiBoard
{
private:
    // SparkyEmojiDisplay *display_;
    Display* display_ = nullptr;
    PowerManager *power_manager_;
    Button boot_button_;
    // Button touch_button_;
    // Button asr_button_;

    void InitializePowerManager()
    {
        // Initialize PowerManager using the configuration pins.
        power_manager_ = new PowerManager(POWER_CHARGE_DETECT_PIN);
    }

    void InitializeSpi()
    {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_SPI_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_SPI_SCK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        // Max transfer size updated for 240x320 display
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        // buscfg.flags = SPICOMMON_BUSFLAG_MASTER;
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    // void InitializeButtons()
    // {
    //     boot_button_.OnClick([this]()
    //                          {
    //         auto& app = Application::GetInstance();
    //         if (app.GetDeviceState() == kDeviceStateStarting &&
    //             !WifiStation::GetInstance().IsConnected()) {
    //             ResetWifiConfiguration();
    //         }
    //         app.ToggleChatState(); });
    // }
    void InitializeButtons()
    {

        // 配置 GPIO
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << BUILTIN_LED_GPIO, // 设置需要配置的 GPIO 引脚
            .mode = GPIO_MODE_OUTPUT,                 // 设置为输出模式
            .pull_up_en = GPIO_PULLUP_DISABLE,        // 禁用上拉
            .pull_down_en = GPIO_PULLDOWN_DISABLE,    // 禁用下拉
            .intr_type = GPIO_INTR_DISABLE            // 禁用中断
        };
        gpio_config(&io_conf); // 应用配置

        boot_button_.OnClick([this]()
                             {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting &&
                !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            gpio_set_level(BUILTIN_LED_GPIO, 1);
            app.ToggleChatState(); });

        // boot_button_.OnDoubleClick([this]() {
        //     auto& app = Application::GetInstance();
        //     if (app.GetDeviceState() == kDeviceStateStarting || app.GetDeviceState() == kDeviceStateWifiConfiguring) {
        //         SwitchNetworkType();
        //     }
        // });

        // asr_button_.OnClick([this]()
        //                     {
        //     std::string wake_word="Hi, Walle";
        //     Application::GetInstance().WakeWordInvoke(wake_word); });

        // touch_button_.OnPressUp([this]()
        //                           {
        //     gpio_set_level(BUILTIN_LED_GPIO, 1);
        //     Application::GetInstance().StartListening(); });

        // touch_button_.OnPressDown([this]()
        //                         {
        //     gpio_set_level(BUILTIN_LED_GPIO, 0);
        //     Application::GetInstance().StopListening(); });
    }

    // IoT initialization, adding devices visible to AI
    void InitializeTools()
    {
        static LampController lamp(LAMP_GPIO);
    }

    void InitializeController()
    {
        // This function initializes the Otto movement logic and MCP tools
        InitializeSparkyController();
    }

    void InitializeSt7789Display()
    {
        ESP_LOGI(TAG, "Init ST7789 display");

        ESP_LOGI(TAG, "Install panel IO");
        esp_lcd_panel_io_handle_t io_handle = NULL;
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_SPI_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = DISPLAY_SPI_MODE;
        io_config.pclk_hz = DISPLAY_SPI_SCLK_HZ;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        io_config.flags.lsb_first = 0;
        io_config.flags.dc_low_on_data = 0;
        io_config.flags.octal_mode = 0;
        io_config.flags.sio_mode = 0;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &io_handle));

        ESP_LOGI(TAG, "Install ST7789 panel driver");
        esp_lcd_panel_handle_t panel_handle = NULL;
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RESET_PIN;
        // panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.rgb_endian = LCD_RGB_ENDIAN_RGB; // LCD_RGB_ENDIAN_RGB;
        panel_config.bits_per_pixel = 16;

        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

        // Configuration for 1.14" display orientation (matching the config.h defines)
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, DISPLAY_SWAP_XY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
        vTaskDelay(pdMS_TO_TICKS(10));
#if CONFIG_USE_EMOTE_MESSAGE_STYLE
        display_ = new emote::EmoteDisplay(panel_handle, io_handle, DISPLAY_WIDTH, DISPLAY_HEIGHT);
#else
        // Create the display object
        display_ = new SparkyEmojiDisplay(io_handle, panel_handle, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                            DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X,
                                            DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
        // display_->SetupHighTempWarningPopup();
#endif
    }

public:
    SPARKY() : boot_button_(BOOT_BUTTON_GPIO)
    {
        InitializeSpi();
        InitializeSt7789Display();
        InitializeButtons();
        InitializePowerManager();
        InitializeController(); // Initialize servo control (Otto)
        InitializeTools();
        if (DISPLAY_BACKLIGHT_PIN != GPIO_NUM_NC)
        {
            GetBacklight()->RestoreBrightness();
        }
    }

    virtual Led *GetLed() override
    {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    // virtual AudioCodec* GetAudioCodec() override {
    //     // Using NoAudioCodecSimplex for boards without a dedicated codec
    //     static NoAudioCodecSimplex audio_codec(
    //         AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
    //         AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT,
    //         AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DIN);
    //     return &audio_codec;
    // }
    virtual AudioCodec *GetAudioCodec() override
    {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        static NoAudioCodecSimplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                                               AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT, AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
#else
        static NoAudioCodecDuplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                                              AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_DOUT, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
#endif
        return &audio_codec;
    }

    virtual Display *GetDisplay() override { return display_; }

    virtual Backlight *GetBacklight() override
    {
        if (DISPLAY_BACKLIGHT_PIN != GPIO_NUM_NC)
        {
            static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
            return &backlight;
        }
        return nullptr;
    }

    virtual bool GetBatteryLevel(int &level, bool &charging, bool &discharging) override
    {
        charging = power_manager_->IsCharging();
        discharging = power_manager_->IsDischarging();
        level = power_manager_->GetBatteryLevel();
        return true;
    }

    virtual bool GetTemperature(float &esp32temp) override
    {
        esp32temp = 0.0f;
        return false;
    }
};

DECLARE_BOARD(SPARKY);