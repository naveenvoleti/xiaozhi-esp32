// fileName: simple_pwm_backlight.h

#ifndef SIMPLE_PWM_BACKLIGHT_H
#define SIMPLE_PWM_BACKLIGHT_H

// #include "led/pwm_backlight.h" // We still include this in case it exists somewhere else
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "backlight.h" // Make sure the base class is included

// Define a simple Backlight class implementation that uses LEDC directly.
// This replaces the missing PwmBacklight class functionality.
class SimplePwmBacklight : public Backlight {
public:
    SimplePwmBacklight(gpio_num_t pin, bool invert) 
        : pin_(pin), invert_(invert), current_brightness_(255) {
        
        // Configuration logic is moved into GetBacklight() in the main board file 
        // to simplify this header, or we define a simple init function here.
        if (pin_ != GPIO_NUM_NC) {
             // Basic LEDC setup (simplified)
            ledc_timer_config_t ledc_timer = {
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .duty_resolution = LEDC_TIMER_8_BIT, // 256 levels
                .timer_num = LEDC_TIMER_0,
                .freq_hz = 5000, // 5 kHz
                .clk_cfg = LEDC_AUTO_CLK
            };
            ledc_timer_config(&ledc_timer);

            ledc_channel_config_t ledc_channel = {
                .gpio_num   = pin_,
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .channel    = LEDC_CHANNEL_0,
                .intr_type  = LEDC_INTR_DISABLE,
                .timer_sel  = LEDC_TIMER_0,
                .duty       = 0,
                .hpoint     = 0,
            };
            ledc_channel_config(&ledc_channel);
        }
    }

    //
    // vvvv ---- THIS IS THE FIX ---- vvvv
    //
    // Renamed from SetBrightness to SetBrightnessImpl to match the base class
    // Added 'override' keyword to ensure it matches.
    virtual void SetBrightnessImpl(uint8_t brightness) override {
        current_brightness_ = brightness;
        if (pin_ != GPIO_NUM_NC) {
            uint32_t duty = invert_ ? (255 - brightness) : brightness;
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        }
    }
    //
    // ^^^^ ---- THIS IS THE FIX ---- ^^^^
    //

    virtual uint8_t GetBrightness() {
        return current_brightness_;
    }

    virtual void RestoreBrightness() {
        // This is correct. It calls the base class SetBrightness,
        // which will then call our new SetBrightnessImpl.
        SetBrightness(255); 
    }

private:
    gpio_num_t pin_;
    bool invert_;
    uint8_t current_brightness_;
};

#endif // SIMPLE_PWM_BACKLIGHT_H