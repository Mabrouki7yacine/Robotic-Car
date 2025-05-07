#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "driver/gpio.h"
#include "driver/ledc.h"

#define TAG "PWM"

#define LEDC_MODE       LEDC_LOW_SPEED_MODE // or LEDC_HIGH_SPEED_MODE depending on your configuration
#define LEDC_TIMER      LEDC_TIMER_0        // Define the timer number
#define LEDC_DUTY_RES   LEDC_TIMER_13_BIT   // Define the duty cycle resolution
#define LEDC_FREQUENCY  5000                // Define the frequency in Hz

void set_up(int rigel, int pin) {
    int timer_num = 0;
    
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = timer_num,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = rigel,
        .timer_sel      = timer_num,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = pin,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void analog_write(int channel, int duty_cycle) {
    int value;
    value = (duty_cycle * ((1 << LEDC_DUTY_RES) - 1)) / 255;
    printf("%d\n", value);
    esp_err_t err = ledc_set_duty(LEDC_MODE, channel, value);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_set_duty failed: %s", esp_err_to_name(err));
        return;
    }

    err = ledc_update_duty(LEDC_MODE, channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_update_duty failed: %s", esp_err_to_name(err));
        return;
    }
}