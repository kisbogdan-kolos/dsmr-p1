#include "esp_log.h"

#include "config.h"

#include "espnow/send.h"
#include "spiffs/datastore.h"
#include "uart/uart.h"
#include "led/led.h"

#ifdef LEAK_DETECTOR
static const char *TAG = "main";

static void leakDetector(void *pvParameters)
{
    while (true)
    {
        ESP_LOGI(TAG, "Free heap: %lu", esp_get_free_heap_size());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
#endif

void app_main(void)
{
    ledInit();
    ledSetColor(RED, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    uartInit();
    sendInit();
    datastoreInit();

#ifdef LEAK_DETECTOR
    xTaskCreate(leakDetector, "leak_detector", 4096, NULL, 0, NULL);
#endif
}
