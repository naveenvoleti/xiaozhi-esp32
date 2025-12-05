#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void motor_ledc_init(void);
void spark_bot_motion_control(float x, float y);
void set_motor_speed_coefficients(float coefficient);
void spark_bot_dance(void);


#ifdef __cplusplus
}
#endif

