#include <stdio.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_websocket_client.h"

#include "config.h"

#include "websocket.h"
#include "espnow/receive.h"
#include "led/led.h"

static const char *TAG = "websocket";

static esp_websocket_client_handle_t client;

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        ledSetColor(GREEN);
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        ledSetColor(CYAN);
        break;
    case WEBSOCKET_EVENT_DATA:
        if (data->op_code == 1)
        {
            char *text = (char *)malloc(data->data_len + 1);
            memcpy(text, data->data_ptr, data->data_len);
            text[data->data_len] = '\0';
            ESP_LOGI(TAG, "Received text data: %s", text);
            uint32_t id;
            if (sscanf(text, "ACK%lu", &id) == 1)
            {
                ESP_LOGI(TAG, "Received id: %lu", id);
                receiveAck(id);
                ledSetColor(BLUE);
            }
            free(text);
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    }
}

static void websocket_app_start(void)
{
    esp_websocket_client_config_t websocket_cfg = {};

    websocket_cfg.uri = WEBSOCKET_URI;
    websocket_cfg.port = WEBSOCKET_PORT;
    websocket_cfg.path = WEBSOCKET_PATH;
    websocket_cfg.reconnect_timeout_ms = WEBSOCKET_RECONNECT_TIMEOUT;
    websocket_cfg.network_timeout_ms = WEBSOCKET_NETWORK_TIMEOUT;

    ESP_LOGI(TAG, "Connecting to %s...", websocket_cfg.uri);

    client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);

    esp_websocket_client_start(client);
}

void websocketInit()
{
    ESP_ERROR_CHECK(esp_netif_init());
    websocket_app_start();
}

void websocketDestroy()
{
    esp_websocket_client_stop(client);
    ESP_LOGI(TAG, "Websocket Stopped");
    esp_websocket_client_destroy(client);
}

void websocketSendData(Data *data)
{
    char *json = telegramToJson(data);
    esp_websocket_client_send_text(client, json, strlen(json), portMAX_DELAY);
    free(json);
}