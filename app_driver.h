#ifndef APP_DRIVER_H
#define APP_DRIVER_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PWM_CHANNEL_0_GPIO 4
#define PWM_CHANNEL_1_GPIO 5
#define PWM_CHANNEL_2_GPIO 6
#define PWM_NUM_CHANNELS   3

#define RGB_LED_GPIO 8

void app_driver_init(void);

void app_driver_set_pwm(uint8_t channel, uint8_t duty);

void app_driver_set_channel_power(uint8_t channel, bool on, uint8_t level);

void app_driver_set_rgb_led(uint8_t red, uint8_t green, uint8_t blue);

#ifdef __cplusplus
}
#endif

#endif
