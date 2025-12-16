#include <cmath>
#include "app_driver.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "app_driver";

#define PWM_FREQUENCY    5000
#define PWM_RESOLUTION   LEDC_TIMER_8_BIT
#define PWM_TIMER        LEDC_TIMER_0
#define PWM_SPEED_MODE   LEDC_LOW_SPEED_MODE

static const gpio_num_t pwm_gpios[PWM_NUM_CHANNELS] = {
    (gpio_num_t)PWM_CHANNEL_0_GPIO,
    (gpio_num_t)PWM_CHANNEL_1_GPIO,
    (gpio_num_t)PWM_CHANNEL_2_GPIO
};

static const ledc_channel_t ledc_channels[PWM_NUM_CHANNELS] = {
    LEDC_CHANNEL_0,
    LEDC_CHANNEL_1,
    LEDC_CHANNEL_2
};

static bool channel_on[PWM_NUM_CHANNELS] = {false, false, false};
static uint8_t channel_level[PWM_NUM_CHANNELS] = {0, 0, 0};

static rmt_channel_handle_t rmt_channel = NULL;
static rmt_encoder_handle_t led_encoder = NULL;

void app_driver_init(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Initializing 3-Channel PWM Driver");
    ESP_LOGI(TAG, "PWM Configuration:");
    ESP_LOGI(TAG, "  Channel 0 -> GPIO %d", PWM_CHANNEL_0_GPIO);
    ESP_LOGI(TAG, "  Channel 1 -> GPIO %d", PWM_CHANNEL_1_GPIO);
    ESP_LOGI(TAG, "  Channel 2 -> GPIO %d", PWM_CHANNEL_2_GPIO);
    ESP_LOGI(TAG, "  Frequency: %d Hz", PWM_FREQUENCY);
    ESP_LOGI(TAG, "  Resolution: 8-bit (0-255)");
    ESP_LOGI(TAG, "  RGB LED -> GPIO %d (WS2812)", RGB_LED_GPIO);
    ESP_LOGI(TAG, "========================================");

    ledc_timer_config_t timer_config = {
        .speed_mode = PWM_SPEED_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = PWM_TIMER,
        .freq_hz = PWM_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));
    ESP_LOGI(TAG, "✓ LEDC timer configured");

    for (int i = 0; i < PWM_NUM_CHANNELS; i++) {
        ledc_channel_config_t channel_config = {
            .gpio_num = pwm_gpios[i],
            .speed_mode = PWM_SPEED_MODE,
            .channel = ledc_channels[i],
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = PWM_TIMER,
            .duty = 0,
            .hpoint = 0
        };
        ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
        ESP_LOGI(TAG, "✓ PWM Channel %d configured on GPIO %d", i, pwm_gpios[i]);
    }

    ESP_LOGI(TAG, "Configuring WS2812B RGB LED on GPIO %d...", RGB_LED_GPIO);
    
    rmt_tx_channel_config_t tx_config = {
        .gpio_num = (gpio_num_t)RGB_LED_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
        .flags = {}
    };
    
    esp_err_t ret = rmt_new_tx_channel(&tx_config, &rmt_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT TX channel: %s", esp_err_to_name(ret));
        rmt_channel = NULL;
    } else {
        ESP_LOGI(TAG, "✓ RMT TX channel created");
        
        rmt_bytes_encoder_config_t encoder_config = {};
        encoder_config.bit0.level0 = 1;
        encoder_config.bit0.duration0 = 3;
        encoder_config.bit0.level1 = 0;
        encoder_config.bit0.duration1 = 9;
        encoder_config.bit1.level0 = 1;
        encoder_config.bit1.duration0 = 9;
        encoder_config.bit1.level1 = 0;
        encoder_config.bit1.duration1 = 3;
        encoder_config.flags.msb_first = 1;
        
        ret = rmt_new_bytes_encoder(&encoder_config, &led_encoder);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create encoder: %s", esp_err_to_name(ret));
            led_encoder = NULL;
        } else {
            ESP_LOGI(TAG, "✓ WS2812 encoder created");
            
            ret = rmt_enable(rmt_channel);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "✓ RMT channel enabled");
                
                app_driver_set_rgb_led(0, 255, 0);
                vTaskDelay(pdMS_TO_TICKS(300));
                app_driver_set_rgb_led(0, 0, 0);
            }
        }
    }

    ESP_LOGI(TAG, "✓ PWM driver initialization complete");
}

void app_driver_set_pwm(uint8_t channel, uint8_t duty)
{
    if (channel >= PWM_NUM_CHANNELS) {
        ESP_LOGW(TAG, "Invalid PWM channel: %d", channel);
        return;
    }
    
    ESP_ERROR_CHECK(ledc_set_duty(PWM_SPEED_MODE, ledc_channels[channel], duty));
    ESP_ERROR_CHECK(ledc_update_duty(PWM_SPEED_MODE, ledc_channels[channel]));
    
    ESP_LOGI(TAG, "PWM Channel %d (GPIO %d): duty = %d/255 (%.1f%%)", 
             channel, pwm_gpios[channel], duty, (duty / 255.0f) * 100.0f);
}

void app_driver_set_channel_power(uint8_t channel, bool on, uint8_t level)
{
    if (channel >= PWM_NUM_CHANNELS) {
        ESP_LOGW(TAG, "Invalid PWM channel: %d", channel);
        return;
    }
    
    channel_on[channel] = on;
    channel_level[channel] = level;
    
    uint8_t duty = on ? level : 0;
    
    ESP_LOGI(TAG, "┌────────────────────────────────────────┐");
    ESP_LOGI(TAG, "│ PWM Channel %d Update                   │", channel);
    ESP_LOGI(TAG, "├────────────────────────────────────────┤");
    ESP_LOGI(TAG, "│ GPIO:    %d                             ", pwm_gpios[channel]);
    ESP_LOGI(TAG, "│ State:   %s                            ", on ? "ON " : "OFF");
    ESP_LOGI(TAG, "│ Level:   %d/254 (%.1f%%)               ", level, (level / 254.0f) * 100.0f);
    ESP_LOGI(TAG, "│ Duty:    %d/255                        ", duty);
    ESP_LOGI(TAG, "└────────────────────────────────────────┘");
    
    app_driver_set_pwm(channel, duty);
}

void app_driver_set_rgb_led(uint8_t red, uint8_t green, uint8_t blue)
{
    if (rmt_channel == NULL || led_encoder == NULL) {
        return;
    }
    
    uint8_t led_data[3] = {green, red, blue};
    
    rmt_transmit_config_t tx_config = {};
    tx_config.loop_count = 0;
    
    esp_err_t ret = rmt_transmit(rmt_channel, led_encoder, led_data, sizeof(led_data), &tx_config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "RGB LED transmit failed: %s", esp_err_to_name(ret));
    }
    
    rmt_tx_wait_all_done(rmt_channel, 100);
}
