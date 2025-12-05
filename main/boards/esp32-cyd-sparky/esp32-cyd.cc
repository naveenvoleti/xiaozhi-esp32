#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "display/emote_display.h"
#include "electron_emoji_display.h"
#include "system_reset.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"
#include "movements.h"
#include "lamp_controller.h"
#include "led/single_led.h"
#include "power_manager.h"
#include "simple_pwm_backlight.h" 

#include <wifi_station.h>
#include <esp_log.h>
#include <driver/spi_common.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_touch.h>
#include <esp_lvgl_port.h>
#include "esp_lcd_touch_xpt2046.h"
#include <driver/i2c_master.h>

#define TAG "CYD_Board"

// Touch pin definitions (typical for CYD)
#ifndef TOUCH_CS_PIN
#define TOUCH_CS_PIN GPIO_NUM_33
#endif

#ifndef TOUCH_IRQ_PIN
#define TOUCH_IRQ_PIN GPIO_NUM_36
#endif


void InitializeElectronBotController();

class CompactWifiBoardLCD : public WifiBoard
{
private:
    Button boot_button_;

    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    esp_lcd_panel_io_handle_t touch_io_ = nullptr;
    esp_lcd_touch_handle_t touch_handle_ = nullptr;
    Display *display_ = nullptr;
    PowerManager* power_manager_;


    void InitializePowerManager() {
        // Initialize PowerManager using the configuration pins.
        power_manager_ = new PowerManager(POWER_CHARGE_DETECT_PIN);
    }

    void InitializeSpi()
    {
        ESP_LOGI(TAG, "Initializing SPI bus");
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_MOSI_PIN;
        buscfg.miso_io_num = DISPLAY_MISO_PIN; // Touch needs MISO
        buscfg.sclk_io_num = DISPLAY_CLK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        buscfg.flags = SPICOMMON_BUSFLAG_MASTER;
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
        ESP_LOGI(TAG, "SPI bus initialized");
    }

    void InitializeLcdDisplay()
    {
        ESP_LOGI(TAG, "Install panel IO");
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = 3;               // SPI mode 3 (important for ST7796)
        io_config.pclk_hz = 40 * 1000 * 1000; // 27 MHz
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        io_config.flags.lsb_first = 0;
        io_config.flags.dc_low_on_data = 0;
        io_config.flags.octal_mode = 0;
        io_config.flags.sio_mode = 0;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &io_config, &panel_io_));

        ESP_LOGI(TAG, "Install ST7796 LCD driver");
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_PIN;
        panel_config.rgb_ele_order = DISPLAY_RGB_ORDER;
        panel_config.bits_per_pixel = 16;
        panel_config.flags.reset_active_high = 0;

        // Initialize ST7796 display (using ST7789 driver - compatible)
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io_, &panel_config, &panel_));
        ESP_LOGI(TAG, "ST7796 LCD driver installed");

        // Reset the display
        ESP_LOGI(TAG, "Resetting display");
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_));
        vTaskDelay(pdMS_TO_TICKS(150)); // Wait after reset

        // Initialize the display
        ESP_LOGI(TAG, "Initializing display");
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_));
        vTaskDelay(pdMS_TO_TICKS(10)); // Wait after init

        // Apply display configurations
        ESP_LOGI(TAG, "Configuring display settings");
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_, DISPLAY_SWAP_XY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y));
        // ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_, DISPLAY_INVERT_COLOR));
        vTaskDelay(pdMS_TO_TICKS(10)); // Wait after configuration

        // CRITICAL: Turn the display ON
        ESP_LOGI(TAG, "Turning display on");
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_, true));
        vTaskDelay(pdMS_TO_TICKS(10)); // Wait after turning on

        // Create display object - using EmoteDisplay for animations
#ifdef CONFIG_USE_EMOTE_MESSAGE_STYLE
        ESP_LOGI(TAG, "Creating EmoteDisplay with animation support");
        display_ = new emote::EmoteDisplay(panel_, panel_io_,
                                           DISPLAY_WIDTH, DISPLAY_HEIGHT);
#else
        ESP_LOGI(TAG, "Creating standard SpiLcdDisplay");
        // display_ = new SpiLcdDisplay(panel_io_, panel_,
        //                              DISPLAY_WIDTH, DISPLAY_HEIGHT,
        //                              DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y,
        //                              DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y,
        //                              DISPLAY_SWAP_XY);
        display_ = new ElectronEmojiDisplay(panel_io_, panel_, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                            DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X,
                                            DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
#endif
        ESP_LOGI(TAG, "Display initialization complete");
    }

    // =========================================================================
    // MODIFIED FUNCTION: InitializeTouch()
    // - CRITICAL: Forced polling mode (int_gpio_num = GPIO_NUM_NC) to bypass potential IRQ issues.
    // - Corrected Unicode/whitespace errors.
    // =========================================================================
    void InitializeTouch()
    {
        ESP_LOGI(TAG, "Initializing XPT2046 touch controller");

        // FIX 1: Custom Touch panel IO configuration with lower PCLK (2.5MHz) and SPI mode 0
        // XPT2046 requires a slow clock and SPI Mode 0, different from the display.
        esp_lcd_panel_io_spi_config_t tp_io_config = {};
        tp_io_config.cs_gpio_num = TOUCH_CS_PIN;
        tp_io_config.dc_gpio_num = GPIO_NUM_NC; // DC is not used for XPT2046
        tp_io_config.spi_mode = 0;               // XPT2046 typically uses SPI mode 0
        tp_io_config.pclk_hz = 2500 * 1000;     // Critical: Set lower SPI frequency (~2.5 MHz)
        tp_io_config.trans_queue_depth = 5;
        tp_io_config.lcd_cmd_bits = 8;
        tp_io_config.lcd_param_bits = 8;
        tp_io_config.on_color_trans_done = NULL;
        tp_io_config.user_ctx = NULL;
        tp_io_config.flags.dc_low_on_data = 0;
        tp_io_config.flags.sio_mode = 0;
        tp_io_config.flags.lsb_first = 0;
        tp_io_config.flags.octal_mode = 0;

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &tp_io_config, &touch_io_));

        // FIX 2: Touch configuration - Orientation settings
        esp_lcd_touch_config_t tp_cfg = {
            .x_max = DISPLAY_WIDTH,
            .y_max = DISPLAY_HEIGHT,
            .rst_gpio_num = GPIO_NUM_NC,
            
            // *** CRITICAL FIX: DISABLE INTERRUPT PIN ***
            // Using GPIO_NUM_NC (Not Connected) is the correct way to disable a GPIO for the driver.
            // This forces polling mode.
            .int_gpio_num = GPIO_NUM_NC, 
            
            .levels = {
                .reset = 0,
                .interrupt = 0,
            },
            .flags = {
                .swap_xy = DISPLAY_SWAP_XY,
                .mirror_x = DISPLAY_MIRROR_X,
                .mirror_y = DISPLAY_MIRROR_Y,
            },
        };

        // Create touch driver
        esp_err_t ret = esp_lcd_touch_new_spi_xpt2046(touch_io_, &tp_cfg, &touch_handle_);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to create XPT2046 touch: %s", esp_err_to_name(ret));
            return;
        }
        ESP_LOGI(TAG, "XPT2046 touch controller initialized (Polling Mode)");

        // Register touch with LVGL (only for standard LVGL display)
#ifndef CONFIG_USE_EMOTE_MESSAGE_STYLE
        const lvgl_port_touch_cfg_t touch_cfg_lvgl = {
            .disp = lv_display_get_default(),
            .handle = touch_handle_,
        };

        lv_indev_t *touch_indev = lvgl_port_add_touch(&touch_cfg_lvgl);
        if (touch_indev == NULL)
        {
            ESP_LOGE(TAG, "Failed to register touch with LVGL");
            return;
        }

        // Can't detect touch? Testing of equipment to be replaced
        // /* read data test */ 
        for (uint8_t i = 0; i < 50; i++) {
            esp_lcd_touch_read_data(touch_handle_);
            if (touch_handle_->data.points > 0) {
                printf("\ntouch: %d, %d\n", touch_handle_->data.coords[0].x, touch_handle_->data.coords[0].y);
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        ESP_LOGI(TAG, "Touch panel registered with LVGL successfully");
#else
        ESP_LOGI(TAG, "Touch initialized (EmoteDisplay uses custom graphics engine). Ensure polling loop is running.");
#endif
    }
    // =========================================================================
    // END OF MODIFIED FUNCTION
    // =========================================================================

    void InitializeButtons()
    {
        // Configure built-in LED GPIO
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << BUILTIN_LED_GPIO,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE};
        gpio_config(&io_conf);

        boot_button_.OnClick([this]()
                             {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            gpio_set_level(BUILTIN_LED_GPIO, 1);
            app.ToggleChatState(); });

        ESP_LOGI(TAG, "Buttons initialized");
    }

    void InitializeTools()
    {
        static LampController lamp(LAMP_GPIO);
        auto &mcp_server = McpServer::GetInstance();
        mcp_server.AddTool("self.system.reconfigure_wifi",
                           "Reboot the device and enter WiFi configuration mode.\n"
                           "**CAUTION** You must ask the user to confirm this action.",
                           PropertyList(), [this](const PropertyList &properties)
                           {
                               ResetWifiConfiguration();
                               return true; });
        ESP_LOGI(TAG, "Tools initialized");
    }

    void InitializeController() { InitializeElectronBotController(); }

public:
    CompactWifiBoardLCD() : boot_button_(BOOT_BUTTON_GPIO)
    {
        ESP_LOGI(TAG, "Initializing CYD Board with Touch and Emote Display");
        InitializeSpi();
        InitializeLcdDisplay();
        InitializeTouch(); // Initialize touch after display
        InitializeController();
        InitializePowerManager();
        InitializeButtons();
        InitializeTools();

        // Restore backlight brightness if available
        if (DISPLAY_BACKLIGHT_PIN != GPIO_NUM_NC)
        {
            auto *backlight = GetBacklight();
            if (backlight)
            {
                backlight->RestoreBrightness();
                ESP_LOGI(TAG, "Backlight restored");
            }
        }
        ESP_LOGI(TAG, "CYD Board initialization complete");
    }

    virtual ~CompactWifiBoardLCD()
    {
        // Clean up touch resources
        if (touch_handle_)
        {
            esp_lcd_touch_del(touch_handle_);
        }
        if (touch_io_)
        {
            esp_lcd_panel_io_del(touch_io_);
        }
    }

    virtual Led *GetLed() override
    {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    virtual AudioCodec *GetAudioCodec() override
    {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        static NoAudioCodecSimplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                                               AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT,
                                               AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
#else
        static NoAudioCodecDuplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                                              AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN);
#endif
        return &audio_codec;
    }

    virtual Display *GetDisplay() override
    {
        return display_;
    }

    // virtual Backlight *GetBacklight() override
    // {
    //     if (DISPLAY_BACKLIGHT_PIN != GPIO_NUM_NC)
    //     {
    //         static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
    //         return &backlight;
    //     }
    //     return nullptr;
    // }

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

    // Optional: Get touch handle for advanced operations
    esp_lcd_touch_handle_t GetTouchHandle()
    {
        return touch_handle_;
    }
};

DECLARE_BOARD(CompactWifiBoardLCD);