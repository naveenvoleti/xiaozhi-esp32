#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "motion_control.h"
#include "config.h"

// Motor pin definitions from config.h
#define M1_IN1  LEDC_M1_CHANNEL_A  // GPIO 21 - Left Motor Forward
#define M1_IN2  LEDC_M1_CHANNEL_B  // GPIO 47 - Left Motor Backward
#define M2_IN3  LEDC_M2_CHANNEL_A  // GPIO 41 - Right Motor Forward
#define M2_IN4  LEDC_M2_CHANNEL_B  // GPIO 42 - Right Motor Backward

// Motor direction control
typedef enum {
    MOTOR_STOP,
    MOTOR_FORWARD,
    MOTOR_BACKWARD
} motor_direction_t;

// Initialize GPIO pins for motor control (replaces motor_ledc_init)
void motor_ledc_init(void)
{
    // Configure all motor control pins as outputs
    gpio_config_t io_conf = {
        .pin_bit_mask = ((1ULL << M1_IN1) | (1ULL << M1_IN2) | 
                         (1ULL << M2_IN3) | (1ULL << M2_IN4)),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    
    // Initialize all pins to LOW (motors stopped)
    gpio_set_level(M1_IN1, 0);
    gpio_set_level(M1_IN2, 0);
    gpio_set_level(M2_IN3, 0);
    gpio_set_level(M2_IN4, 0);
    
    printf("Motor GPIO initialized - Simple binary control (HIGH/LOW)\n");
    printf("  Left Motor:  IN1=GPIO%d, IN2=GPIO%d\n", M1_IN1, M1_IN2);
    printf("  Right Motor: IN3=GPIO%d, IN4=GPIO%d\n", M2_IN3, M2_IN4);
}

// Internal function to control left motor
static void set_left_motor(motor_direction_t direction)
{
    switch(direction) {
        case MOTOR_FORWARD:
            gpio_set_level(M1_IN1, 1);  // HIGH
            gpio_set_level(M1_IN2, 0);  // LOW
            break;
        case MOTOR_BACKWARD:
            gpio_set_level(M1_IN1, 0);  // LOW
            gpio_set_level(M1_IN2, 1);  // HIGH
            break;
        case MOTOR_STOP:
        default:
            gpio_set_level(M1_IN1, 0);  // LOW
            gpio_set_level(M1_IN2, 0);  // LOW
            break;
    }
}

// Internal function to control right motor
// NOTE: Motor 2 directions are swapped due to reverse wiring
static void set_right_motor(motor_direction_t direction)
{
    switch(direction) {
        case MOTOR_FORWARD:
            gpio_set_level(M2_IN3, 0);  // LOW (swapped)
            gpio_set_level(M2_IN4, 1);  // HIGH (swapped)
            break;
        case MOTOR_BACKWARD:
            gpio_set_level(M2_IN3, 1);  // HIGH (swapped)
            gpio_set_level(M2_IN4, 0);  // LOW (swapped)
            break;
        case MOTOR_STOP:
        default:
            gpio_set_level(M2_IN3, 0);  // LOW
            gpio_set_level(M2_IN4, 0);  // LOW
            break;
    }
}

// Main motion control function based on x, y coordinates
void spark_bot_motion_control(float x, float y)
{
    // Threshold for detecting movement commands
    const float threshold = 0.1;
    
    // Forward/Backward control (y-axis)
    if (y > threshold && x > -threshold && x < threshold) {
        // Pure forward
        set_left_motor(MOTOR_FORWARD);
        set_right_motor(MOTOR_FORWARD);
        printf("Motion: FORWARD\n");
    }
    else if (y < -threshold && x > -threshold && x < threshold) {
        // Pure backward
        set_left_motor(MOTOR_BACKWARD);
        set_right_motor(MOTOR_BACKWARD);
        printf("Motion: BACKWARD\n");
    }
    // Turn control (x-axis)
    else if (x > threshold && y > -threshold && y < threshold) {
        // Pure right turn (tank turn)
        set_left_motor(MOTOR_FORWARD);
        set_right_motor(MOTOR_BACKWARD);
        printf("Motion: TURN RIGHT\n");
    }
    else if (x < -threshold && y > -threshold && y < threshold) {
        // Pure left turn (tank turn)
        set_left_motor(MOTOR_BACKWARD);
        set_right_motor(MOTOR_FORWARD);
        printf("Motion: TURN LEFT\n");
    }
    // Combined movement (forward/backward + turn)
    else if (y > threshold && x > threshold) {
        // Forward + Right turn (right motor slower/stopped)
        set_left_motor(MOTOR_FORWARD);
        set_right_motor(MOTOR_STOP);
        printf("Motion: FORWARD-RIGHT\n");
    }
    else if (y > threshold && x < -threshold) {
        // Forward + Left turn (left motor slower/stopped)
        set_left_motor(MOTOR_STOP);
        set_right_motor(MOTOR_FORWARD);
        printf("Motion: FORWARD-LEFT\n");
    }
    else if (y < -threshold && x > threshold) {
        // Backward + Right turn
        set_left_motor(MOTOR_BACKWARD);
        set_right_motor(MOTOR_STOP);
        printf("Motion: BACKWARD-RIGHT\n");
    }
    else if (y < -threshold && x < -threshold) {
        // Backward + Left turn
        set_left_motor(MOTOR_STOP);
        set_right_motor(MOTOR_BACKWARD);
        printf("Motion: BACKWARD-LEFT\n");
    }
    // Stop
    else {
        set_left_motor(MOTOR_STOP);
        set_right_motor(MOTOR_STOP);
        printf("Motion: STOP\n");
    }
}

// Motor speed coefficient adjustment (no effect in binary control, kept for compatibility)
void set_motor_speed_coefficients(float coefficient)
{
    // In binary control mode, this function has no effect
    // Kept for API compatibility
    printf("Note: Speed coefficients not applicable in binary GPIO control mode\n");
    printf("Motors always run at full speed (determined by enable pins)\n");
}

// Dance routine for the robot
void spark_bot_dance(void)
{
    printf("\n=== Starting Dance Routine ===\n");
    
    // Move 1: Forward
    printf("Dance Move 1: Forward\n");
    set_left_motor(MOTOR_FORWARD);
    set_right_motor(MOTOR_FORWARD);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    // Move 2: Turn left
    printf("Dance Move 2: Turn Left\n");
    set_left_motor(MOTOR_BACKWARD);
    set_right_motor(MOTOR_FORWARD);
    vTaskDelay(800 / portTICK_PERIOD_MS);
    
    // Move 3: Backward
    printf("Dance Move 3: Backward\n");
    set_left_motor(MOTOR_BACKWARD);
    set_right_motor(MOTOR_BACKWARD);
    vTaskDelay(1200 / portTICK_PERIOD_MS);
    
    // Move 4: Turn right
    printf("Dance Move 4: Turn Right\n");
    set_left_motor(MOTOR_FORWARD);
    set_right_motor(MOTOR_BACKWARD);
    vTaskDelay(800 / portTICK_PERIOD_MS);
    
    // Move 5: Forward
    printf("Dance Move 5: Forward\n");
    set_left_motor(MOTOR_FORWARD);
    set_right_motor(MOTOR_FORWARD);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    // Move 6: Turn right
    printf("Dance Move 6: Turn Right\n");
    set_left_motor(MOTOR_FORWARD);
    set_right_motor(MOTOR_BACKWARD);
    vTaskDelay(800 / portTICK_PERIOD_MS);
    
    // Move 7: Backward
    printf("Dance Move 7: Backward\n");
    set_left_motor(MOTOR_BACKWARD);
    set_right_motor(MOTOR_BACKWARD);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    // Move 8: Turn left
    printf("Dance Move 8: Turn Left\n");
    set_left_motor(MOTOR_BACKWARD);
    set_right_motor(MOTOR_FORWARD);
    vTaskDelay(800 / portTICK_PERIOD_MS);
    
    // Move 9: Stop
    printf("Dance Move 9: Pause\n");
    set_left_motor(MOTOR_STOP);
    set_right_motor(MOTOR_STOP);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    
    // Reverse movements to return to start
    printf("\n--- Returning to Start ---\n");
    
    // Move 10: Turn right
    printf("Return Move 1: Turn Right\n");
    set_left_motor(MOTOR_FORWARD);
    set_right_motor(MOTOR_BACKWARD);
    vTaskDelay(800 / portTICK_PERIOD_MS);
    
    // Move 11: Forward
    printf("Return Move 2: Forward\n");
    set_left_motor(MOTOR_FORWARD);
    set_right_motor(MOTOR_FORWARD);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    // Move 12: Turn left
    printf("Return Move 3: Turn Left\n");
    set_left_motor(MOTOR_BACKWARD);
    set_right_motor(MOTOR_FORWARD);
    vTaskDelay(800 / portTICK_PERIOD_MS);
    
    // Move 13: Backward
    printf("Return Move 4: Backward\n");
    set_left_motor(MOTOR_BACKWARD);
    set_right_motor(MOTOR_BACKWARD);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    // Move 14: Turn left
    printf("Return Move 5: Turn Left\n");
    set_left_motor(MOTOR_BACKWARD);
    set_right_motor(MOTOR_FORWARD);
    vTaskDelay(800 / portTICK_PERIOD_MS);
    
    // Move 15: Forward
    printf("Return Move 6: Forward\n");
    set_left_motor(MOTOR_FORWARD);
    set_right_motor(MOTOR_FORWARD);
    vTaskDelay(1200 / portTICK_PERIOD_MS);
    
    // Move 16: Turn right
    printf("Return Move 7: Turn Right\n");
    set_left_motor(MOTOR_FORWARD);
    set_right_motor(MOTOR_BACKWARD);
    vTaskDelay(800 / portTICK_PERIOD_MS);
    
    // Move 17: Backward
    printf("Return Move 8: Backward\n");
    set_left_motor(MOTOR_BACKWARD);
    set_right_motor(MOTOR_BACKWARD);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    // Final stop
    printf("Dance Move: STOP\n");
    set_left_motor(MOTOR_STOP);
    set_right_motor(MOTOR_STOP);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    
    printf("=== Dance Routine Complete! ===\n\n");
}

// #include <stdio.h>
// #include "math.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/ledc.h"
// #include "esp_err.h"
// #include "motion_control.h"
// #include "config.h"  // Include config for pin definitions

// // Define LEDC (PWM) parameters
// #define LEDC_TIMER                  LEDC_TIMER_0
// #define LEDC_MODE                   LEDC_LOW_SPEED_MODE
// #define LEDC_DUTY_RES               LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
// #define LEDC_FREQUENCY              (4000) // Frequency in Hertz. Set frequency at 4 kHz

// #define LEDC_CHANNEL_COUNT          (4)
// #define LEDC_M1_CHANNEL_A_CH        LEDC_CHANNEL_0
// #define LEDC_M1_CHANNEL_B_CH        LEDC_CHANNEL_1
// #define LEDC_M2_CHANNEL_A_CH        LEDC_CHANNEL_2
// #define LEDC_M2_CHANNEL_B_CH        LEDC_CHANNEL_3

// // Use pin definitions from config.h
// #define LEDC_M1_CHANNEL_A_IO        LEDC_M1_CHANNEL_A
// #define LEDC_M1_CHANNEL_B_IO        LEDC_M1_CHANNEL_B
// #define LEDC_M2_CHANNEL_A_IO        LEDC_M2_CHANNEL_A
// #define LEDC_M2_CHANNEL_B_IO        LEDC_M2_CHANNEL_B

// float m1_coefficient = 1.0;
// float m2_coefficient = 1.0;

// void motor_ledc_init(void)
// {
//     // Prepare and then apply the LEDC PWM timer configuration
//     ledc_timer_config_t ledc_timer = {
//         .speed_mode       = LEDC_MODE,
//         .timer_num        = LEDC_TIMER,
//         .duty_resolution  = LEDC_DUTY_RES,
//         .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 4 kHz
//         .clk_cfg          = LEDC_AUTO_CLK
//     };
//     ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

//     // Array of channel configurations for easy iteration
//     const uint8_t motor_ledc_channel[LEDC_CHANNEL_COUNT] = {
//         LEDC_M1_CHANNEL_A_CH, 
//         LEDC_M1_CHANNEL_B_CH, 
//         LEDC_M2_CHANNEL_A_CH, 
//         LEDC_M2_CHANNEL_B_CH
//     };
//     const int32_t ledc_channel_pins[LEDC_CHANNEL_COUNT] = {
//         LEDC_M1_CHANNEL_A_IO, 
//         LEDC_M1_CHANNEL_B_IO, 
//         LEDC_M2_CHANNEL_A_IO, 
//         LEDC_M2_CHANNEL_B_IO
//     };
    
//     for (int i = 0; i < LEDC_CHANNEL_COUNT; i++) {
//         ledc_channel_config_t ledc_channel = {
//             .speed_mode     = LEDC_MODE,
//             .channel        = motor_ledc_channel[i],
//             .timer_sel      = LEDC_TIMER,
//             .intr_type      = LEDC_INTR_DISABLE,
//             .gpio_num       = ledc_channel_pins[i],
//             .duty           = 0, // Set duty to 0%
//             .hpoint         = 0
//         };
//         ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
//     }
// }

// static void set_left_motor_speed(int speed)
// {
//     if (speed >= 0) {
//         uint32_t m1a_duty = (uint32_t)((speed * m1_coefficient * 8192) / 100);
//         ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M1_CHANNEL_A_CH, m1a_duty));
//         ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M1_CHANNEL_A_CH));
//         ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M1_CHANNEL_B_CH, 0));
//         ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M1_CHANNEL_B_CH));
//     } else {
//         uint32_t m1b_duty = (uint32_t)((-speed * m1_coefficient * 8192) / 100);
//         ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M1_CHANNEL_A_CH, 0));
//         ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M1_CHANNEL_A_CH));
//         ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M1_CHANNEL_B_CH, m1b_duty));
//         ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M1_CHANNEL_B_CH));
//     }
// }

// static void set_right_motor_speed(int speed)
// {
//     if (speed >= 0) {
//         uint32_t m2a_duty = (uint32_t)((speed * m2_coefficient * 8192) / 100);
//         ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M2_CHANNEL_A_CH, m2a_duty));
//         ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M2_CHANNEL_A_CH));
//         ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M2_CHANNEL_B_CH, 0));
//         ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M2_CHANNEL_B_CH));
//     } else {
//         uint32_t m2b_duty = (uint32_t)((-speed * m2_coefficient * 8192) / 100);
//         ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M2_CHANNEL_A_CH, 0));
//         ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M2_CHANNEL_A_CH));
//         ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M2_CHANNEL_B_CH, m2b_duty));
//         ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M2_CHANNEL_B_CH));
//     }
// }

// void spark_bot_motion_control(float x, float y)
// {
//     float base_speed = y * MOTOR_SPEED_MAX;
//     float turn_adjust = x * MOTOR_SPEED_MAX;

//     int left_speed = (int)(base_speed + turn_adjust);
//     int right_speed = (int)(base_speed - turn_adjust);

//     if (left_speed > MOTOR_SPEED_MAX) left_speed = MOTOR_SPEED_MAX;
//     if (left_speed < -MOTOR_SPEED_MAX) left_speed = -MOTOR_SPEED_MAX;
//     if (right_speed > MOTOR_SPEED_MAX) right_speed = MOTOR_SPEED_MAX;
//     if (right_speed < -MOTOR_SPEED_MAX) right_speed = -MOTOR_SPEED_MAX;

//     set_left_motor_speed(left_speed);
//     set_right_motor_speed(right_speed);
// }

// void set_motor_speed_coefficients(float coefficient)
// {
//     if (coefficient < 0) {
//         if (m1_coefficient >= 0.7) {
//             m1_coefficient -= 0.025;
//         }
//         m2_coefficient = 1.0;
//     } else {
//         if (m2_coefficient >= 0.7) {
//             m2_coefficient -= 0.025;
//         }
//         m1_coefficient = 1.0;
//     }
//     printf("m1_coefficient = %f, m2_coefficient = %f\n", m1_coefficient, m2_coefficient);
// }

// typedef struct {
//     int         left_speed;
//     int         right_speed;
//     uint16_t    time;
// } dance_motion_t;

// void spark_bot_dance(void)
// {
//     // Define dance moves using left and right crawler speed and time
//     dance_motion_t dance_moves[] = {
//         // 第一段舞蹈
//         {MOTOR_SPEED_MAX, MOTOR_SPEED_MAX, 1000},  // Move forward quickly for 1 second
//         {MOTOR_SPEED_80, -MOTOR_SPEED_80, 800},    // 左转，0.8 秒
//         {-MOTOR_SPEED_60, -MOTOR_SPEED_60, 1200},  // 向后慢速移动 1.2 秒
//         {-MOTOR_SPEED_80, MOTOR_SPEED_80, 800},    // 右转，0.8 秒
//         {MOTOR_SPEED_80, MOTOR_SPEED_80, 1000},    // 向前中速移动 1 秒
//         {-MOTOR_SPEED_80, MOTOR_SPEED_80, 800},    // 右转，0.8 秒
//         {-MOTOR_SPEED_MAX, -MOTOR_SPEED_MAX, 1000},// 向后快速移动 1 秒
//         {MOTOR_SPEED_80, -MOTOR_SPEED_80, 800},    // 左转，0.8 秒
//         {0, 0, 500},                               // 停止，0.5 秒

//         // Reverse actions to ensure returning to the starting point
//         {-MOTOR_SPEED_80, MOTOR_SPEED_80, 800},    // 右转，0.8 秒
//         {MOTOR_SPEED_MAX, MOTOR_SPEED_MAX, 1000},  // 向前快速移动 1 秒
//         {MOTOR_SPEED_80, -MOTOR_SPEED_80, 800},    // 左转，0.8 秒
//         {-MOTOR_SPEED_80, -MOTOR_SPEED_80, 1000},  // 向后中速移动 1 秒
//         {MOTOR_SPEED_80, -MOTOR_SPEED_80, 800},    // 左转，0.8 秒
//         {MOTOR_SPEED_60, MOTOR_SPEED_60, 1200},    // 向前慢速移动 1.2 秒
//         {-MOTOR_SPEED_80, MOTOR_SPEED_80, 800},    // 右转，0.8 秒
//         {-MOTOR_SPEED_MAX, -MOTOR_SPEED_MAX, 1000},// 向后快速移动 1 秒
//         {0, 0, 500},                               // 停止，0.5 秒
//     };

//     // Iterate through the list of dance moves and execute them
//     for (int i = 0; i < sizeof(dance_moves) / sizeof(dance_moves[0]); i++) {
//         // Control left and right motor speeds
//         set_left_motor_speed(dance_moves[i].left_speed);
//         set_right_motor_speed(dance_moves[i].right_speed);

//         // Hold for the specified time
//         vTaskDelay(dance_moves[i].time / portTICK_PERIOD_MS);

//         // Stop the PWM output of the motors
//         set_left_motor_speed(0);
//         set_right_motor_speed(0);
//     }
// }