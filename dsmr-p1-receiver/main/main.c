#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"

#include "config.h"

#include "espnow/receive.h"
#include "espnow/wifi.h"
#include "websocket/websocket.h"
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

#ifdef CPU_MONITOR
static void cpuMonitor(void *pvParameters)
{
    while (true)
    {
        char *buf = (char *)malloc(1024);
        vTaskGetRunTimeStats(buf);
        ESP_LOGI(TAG, "CPU stats:\n%s", buf);
        free(buf);

        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
#endif

static void wifiReconnectTask(void *pvParameters)
{
    for (;;)
    {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        wifiReconnect();
    }
}

void app_main(void)
{
    ledInit();
    ledSetColor(RED, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ledSetColor(YELLOW, 0);
    SemaphoreHandle_t wifiGotIp = xSemaphoreCreateBinary();
    receiveInit(wifiGotIp);

    xTaskCreate(wifiReconnectTask, "wifi_reconnect", 4096, NULL, 0, NULL);

    while (xSemaphoreTake(wifiGotIp, portMAX_DELAY) != pdTRUE)
        ;
    websocketInit();

#ifdef LEAK_DETECTOR
    xTaskCreate(leakDetector, "leak_detector", 4096, NULL, 0, NULL);
#endif

#ifdef CPU_MONITOR
    xTaskCreate(cpuMonitor, "cpu_monitor", 4096, NULL, 0, NULL);
#endif
}
