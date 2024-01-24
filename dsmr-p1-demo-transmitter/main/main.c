#include "esp_log.h"
#include "esp_system.h"
#include "esp_random.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h"

#include "espnow/send.h"
#include "led/led.h"

static const char *TAG = "main";

#ifdef LEAK_DETECTOR
static void leakDetector(void *pvParameters)
{
    while (true)
    {
        ESP_LOGI(TAG, "Free heap: %lu", esp_get_free_heap_size());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
#endif

static void sendTask(void *pvParameters)
{
    for (;;)
    {
        Data *data = (Data *)malloc(sizeof(Data));
        data->id = esp_random();
        ESP_LOGI(TAG, "Data id: %lu", data->id);
        sendData(data);
        vTaskDelay(DEMO_SEND_INTERVAL / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    ledInit();
    ledSetColor(RED);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    sendInit();

    xTaskCreate(sendTask, "send_task", 4096, NULL, 0, NULL);

#ifdef LEAK_DETECTOR
    xTaskCreate(leakDetector, "leak_detector", 4096, NULL, 0, NULL);
#endif
}
