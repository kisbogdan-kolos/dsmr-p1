#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"
#include "led_strip.h"

#include "config.h"

#include "led.h"

static const char *TAG = "led";

#ifdef LED_TWO_LEDS
static const uint8_t numPixels = 2;
static LedColor color[2] = {OFF, OFF};
#else
static const uint8_t numPixels = 1;
static LedColor color[1] = {OFF};
#endif

static uint8_t brightness = 0;
static adc_oneshot_unit_handle_t adc1_handle;
static led_strip_handle_t led_strip;

static void updateColor()
{
    for (uint8_t i = 0; i < numPixels; i++)
    {
        led_strip_set_pixel(led_strip, i, ((color[i] & 0b100) >> 2) * brightness, ((color[i] & 0b010) >> 1) * brightness, (color[i] & 0b001) * brightness);
    }
    led_strip_refresh(led_strip);
}

static void brightnessAdjustTask(void *pvParameters)
{
    vTaskDelay(100 / portTICK_PERIOD_MS);
    while (1)
    {
        led_strip_clear(led_strip);
        vTaskDelay(100 / portTICK_PERIOD_MS);

        int adcReading = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_2, &adcReading));

        brightness = adcReading >> 6;
        brightness += 3;

        ESP_LOGI(TAG, "Brightness: %hhu", brightness);

        updateColor();

        vTaskDelay(configTICK_RATE_HZ * BRIGHTNESS_ADJUST_INTERVAL);
    }
}

void ledInit()
{
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, LIGHT_SENSOR, &config));

    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_PIN,
        .max_leds = numPixels,
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    xTaskCreate(brightnessAdjustTask, "brightness_adjust", 4096, NULL, 2, NULL);
}

void ledSetColor(LedColor newColor, uint8_t idx)
{
    if (idx >= numPixels)
        return;
    color[idx] = newColor;
    updateColor();
}
