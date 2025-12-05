#include "BuiltinLed.h"
#include <cstring>
#include <string>
#include <algorithm>
#include "esp_log.h"
#include "config.h"
#include <led_strip.h>

#define TAG "builtin_led"

BuiltinLed::BuiltinLed() {
    mutex_ = xSemaphoreCreateMutex();
    blink_event_group_ = xEventGroupCreate();
    xEventGroupSetBits(blink_event_group_, BLINK_TASK_STOPPED_BIT);

    Configure();
    SetGrey();
}

BuiltinLed::~BuiltinLed() {
    StopBlinkInternal();
    if (led_strip_ != nullptr) {
        led_strip_del(led_strip_);
    }
    vSemaphoreDelete(mutex_);
    vEventGroupDelete(blink_event_group_);
}

BuiltinLed& BuiltinLed::GetInstance() {
    static BuiltinLed instance;
    return instance;
}

void BuiltinLed::Configure() {
    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = RGB_LED_GPIO;
    strip_config.max_leds = 1;
    strip_config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB;
    // strip_config.led_pixel_format = LED_PIXEL_FORMAT_GRB;
    strip_config.led_model = LED_MODEL_WS2812;

    led_strip_rmt_config_t rmt_config = {};
    rmt_config.resolution_hz = 10 * 1000 * 1000; // 10MHz

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip_));
    led_strip_clear(led_strip_);
}

void BuiltinLed::SetColor(uint8_t r, uint8_t g, uint8_t b) {
    r_ = r;
    g_ = g;
    b_ = b;
}

void BuiltinLed::TurnOn() {
    StopBlinkInternal();
    xSemaphoreTake(mutex_, portMAX_DELAY);
    led_strip_set_pixel(led_strip_, 0, r_, g_, b_);
    led_strip_refresh(led_strip_);
    is_on_ = true;
    xSemaphoreGive(mutex_);
}

void BuiltinLed::TurnOff() {
    StopBlinkInternal();
    xSemaphoreTake(mutex_, portMAX_DELAY);
    led_strip_clear(led_strip_);
    is_on_ = false;
    xSemaphoreGive(mutex_);
}

void BuiltinLed::BlinkOnce() {
    Blink(1, 100);
}

void BuiltinLed::Blink(int times, int interval_ms) {
    StartBlinkTask(times, interval_ms);
}

void BuiltinLed::StartContinuousBlink(int interval_ms) {
    StartBlinkTask(BLINK_INFINITE, interval_ms);
}

void BuiltinLed::SetBrightness(uint8_t brightness) {
    // Preserve the color ratios when adjusting brightness
    if (r_ == 0 && g_ == 0 && b_ == 0) {
        // If LED is off, set to white at specified brightness
        r_ = brightness;
        g_ = brightness;
        b_ = brightness;
    } else {
        // Scale existing colors proportionally
        float max_component = std::max({r_, g_, b_});
        float scale = brightness / max_component;
        r_ = static_cast<uint8_t>(r_ * scale);
        g_ = static_cast<uint8_t>(g_ * scale);
        b_ = static_cast<uint8_t>(b_ * scale);
    }
}

std::string BuiltinLed::GetState() const {
    bool is_blinking = should_blink_ && (blink_task_ != nullptr);
    bool is_led_on = is_on_ || is_blinking;
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), 
             "{\"r\": %d, \"g\": %d, \"b\": %d, \"is_on\": %s, \"is_blinking\": %s, \"blink_interval\": %d}",
             r_, g_, b_, 
             is_led_on ? "true" : "false",
             is_blinking ? "true" : "false",
             blink_interval_ms_);
    
    return std::string(buffer);
}

void BuiltinLed::StartBlinkTask(int times, int interval_ms) {
    StopBlinkInternal();
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    blink_times_ = times;
    blink_interval_ms_ = interval_ms;
    should_blink_ = true;
    is_on_ = true;

    xEventGroupClearBits(blink_event_group_, BLINK_TASK_STOPPED_BIT);
    xEventGroupSetBits(blink_event_group_, BLINK_TASK_RUNNING_BIT);

    xTaskCreate([](void* obj) {
        auto this_ = static_cast<BuiltinLed*>(obj);
        int count = 0;
        while (this_->should_blink_ && (this_->blink_times_ == BLINK_INFINITE || count < this_->blink_times_)) {
            xSemaphoreTake(this_->mutex_, portMAX_DELAY);
            led_strip_set_pixel(this_->led_strip_, 0, this_->r_, this_->g_, this_->b_);
            led_strip_refresh(this_->led_strip_);
            xSemaphoreGive(this_->mutex_);

            vTaskDelay(this_->blink_interval_ms_ / portTICK_PERIOD_MS);
            if (!this_->should_blink_) break;

            xSemaphoreTake(this_->mutex_, portMAX_DELAY);
            led_strip_clear(this_->led_strip_);
            xSemaphoreGive(this_->mutex_);

            vTaskDelay(this_->blink_interval_ms_ / portTICK_PERIOD_MS);
            if (this_->blink_times_ != BLINK_INFINITE) count++;
        }
        this_->blink_task_ = nullptr;
        this_->is_on_ = false;
        xEventGroupClearBits(this_->blink_event_group_, BLINK_TASK_RUNNING_BIT);
        xEventGroupSetBits(this_->blink_event_group_, BLINK_TASK_STOPPED_BIT);
        vTaskDelete(NULL);
    }, "blink", 2048, this, tskIDLE_PRIORITY, &blink_task_);

    xSemaphoreGive(mutex_);
}

void BuiltinLed::StopBlinkInternal() {
    should_blink_ = false;
    xEventGroupWaitBits(blink_event_group_, BLINK_TASK_STOPPED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
}