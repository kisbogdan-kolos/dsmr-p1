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

#include "send.h"
#include "espnow-dsmr.h"
#include "spiffs/datastore.h"
#include "led/led.h"

static const char *TAG = "espnow-send";

static QueueHandle_t s_example_espnow_queue;
static QueueHandle_t dataQueueHandle;
// f4:12:fa:18:73:ac
// f4:12:fa:15:c8:94
static uint8_t destMac[6] = ESPNOW_DEST_MAC;
static Data *currentData = NULL;
static uint8_t sendAttempts = 0;

static void dsmr_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, 0));

    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_LR));
}

static void dsmr_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    dsmr_espnow_event_t evt;
    dsmr_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

    if (mac_addr == NULL)
    {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    evt.id = EXAMPLE_ESPNOW_SEND_CB;
    memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    send_cb->status = status;
    if (xQueueSend(s_example_espnow_queue, &evt, 512) != pdTRUE)
    {
        ESP_LOGW(TAG, "Send send queue fail");
    }
}

static void dsmr_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    dsmr_espnow_event_t evt;
    dsmr_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
    uint8_t *mac_addr = recv_info->src_addr;

    if (mac_addr == NULL || data == NULL || len <= 0)
    {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    evt.id = EXAMPLE_ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv_cb->data = malloc(len);
    if (recv_cb->data == NULL)
    {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    if (xQueueGenericSend(s_example_espnow_queue, &evt, 512, queueSEND_TO_FRONT) != pdTRUE)
    {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(recv_cb->data);
    }
}

static void dsmr_espnow_task(void *pvParameter)
{
    dsmr_espnow_event_t evt;
    evt.id = EXAMPLE_ESPNOW_SEND_CB;

    while (true)
    {
        if (currentData == NULL)
        {
            xQueueReceive(dataQueueHandle, &currentData, 0);
            if (currentData != NULL)
            {
                ledSetColor(BLUE);
            }
        }

        if (currentData != NULL && uxQueueMessagesWaiting(s_example_espnow_queue) == 0)
        {
            evt.id = EXAMPLE_ESPNOW_SEND_CB;
            xQueueSend(s_example_espnow_queue, &evt, 0);
        }

        if (xQueueReceive(s_example_espnow_queue, &evt, 1000 / portTICK_PERIOD_MS) == pdTRUE)
        {
            switch (evt.id)
            {
            case EXAMPLE_ESPNOW_RECV_CB:
            {
                dsmr_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

                if (recv_cb->data_len == sizeof(dsmr_espnow_payload_recv))
                {
                    dsmr_espnow_payload_recv *payload = (dsmr_espnow_payload_recv *)recv_cb->data;
                    uint16_t calc_crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)&(payload->id), sizeof(uint32_t));
                    if (payload->crc == calc_crc)
                    {
                        if (currentData != NULL && currentData->id == payload->id)
                        {
                            ESP_LOGI(TAG, "Received data id: %lu", payload->id);
                            free(currentData);
                            currentData = NULL;
                            ledSetColor(GREEN);
                            sendAttempts = 0;
                        }
                        else if (currentData != NULL)
                        {
                            ESP_LOGE(TAG, "Received data id: %lu, but expected id: %lu", payload->id, currentData->id);
                        }
                        else
                        {
                            ESP_LOGE(TAG, "Received data id: %lu, but no data to compare", payload->id);
                        }
                    }
                    else
                    {
                        ESP_LOGE(TAG, "CRC mismatch");
                    }
                }
                else
                {
                    ESP_LOGE(TAG, "Received data with wrong size, expected %u, got %d", sizeof(dsmr_espnow_payload_recv), recv_cb->data_len);
                }

                free(recv_cb->data);

                break;
            }
            case EXAMPLE_ESPNOW_SEND_CB:
            {
                if (currentData != NULL)
                {
                    dsmr_espnow_payload_send *payload = (dsmr_espnow_payload_send *)malloc(sizeof(dsmr_espnow_payload_send));
                    memcpy(&(payload->data), currentData, sizeof(Data));
                    payload->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)&(payload->data), sizeof(Data));

                    if (esp_now_send(destMac, (const uint8_t *)payload, sizeof(dsmr_espnow_payload_send)) != ESP_OK)
                    {
                        ESP_LOGE(TAG, "Send error, rebooting...");
                        ledSetColor(RED);

                        xQueueSend(dataQueueHandle, &currentData, 0);
                        currentData = NULL;
                        datastoreSave(dataQueueHandle, uxQueueMessagesWaiting(dataQueueHandle));

                        vTaskDelay(10000 / portTICK_PERIOD_MS);
                        esp_restart();
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Send data id: %lu", payload->data.id);
                        sendAttempts++;
                    }

                    free(payload);
                }

                if (sendAttempts >= 5)
                {
                    ledSetColor(YELLOW);
                    ESP_LOGW(TAG, "Not getting response, slowing send rate");

                    vTaskDelay(60000 / portTICK_PERIOD_MS);

                    if (sendAttempts > 254)
                        sendAttempts = 254;
                }
                else
                {
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                }

                break;
            }
            default:
                ESP_LOGE(TAG, "Callback type error: %d", evt.id);
                break;
            }
        }
    }
}

static esp_err_t dsmr_espnow_init(void)
{
    s_example_espnow_queue = xQueueCreate(10, sizeof(dsmr_espnow_event_t));
    if (s_example_espnow_queue == NULL)
    {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(dsmr_espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(dsmr_espnow_recv_cb));
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)ESPNOW_PMK_KEY));

    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL)
    {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(s_example_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = ESPNOW_CHANNEL;
    peer->ifidx = WIFI_IF_STA;
    peer->encrypt = ESPNOW_ENCRYPT;
    strncpy((char *)peer->lmk, ESPNOW_LMK_KEY, ESP_NOW_KEY_LEN);
    memcpy(peer->peer_addr, destMac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);

    xTaskCreate(dsmr_espnow_task, "example_espnow_task", 2048, NULL, 4, NULL);

    return ESP_OK;
}

static void saveDataTask(void *pvParameter)
{
    while (true)
    {
        vTaskDelay(configTICK_RATE_HZ * DATA_QUEUE_SAVE_TIME);
        size_t cnt = uxQueueMessagesWaiting(dataQueueHandle);
        if (cnt >= DATA_QUEUE_SAVE_LIMIT)
        {
            datastoreSave(dataQueueHandle, DATA_QUEUE_SAVE_SIZE);
        }
        else if (cnt == 0)
        {
            datastoreRead(dataQueueHandle, DATA_QUEUE_READ_SIZE);
        }
    }
}

void sendInit()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    dataQueueHandle = xQueueCreate(DATA_QUEUE_LENGTH, sizeof(Data *));
    if (dataQueueHandle == NULL)
    {
        ESP_LOGE(TAG, "Create data queue fail");
        return;
    }

    dsmr_wifi_init();
    dsmr_espnow_init();

    xTaskCreate(saveDataTask, "saveDataTask", 2048, NULL, 1, NULL);

    ledSetColor(MAGENTA);
}

void sendQueueData(Data *data)
{
    if (xQueueSend(dataQueueHandle, &data, 0) != pdTRUE)
    {
        ESP_LOGE(TAG, "Send data to queue fail");
        free(data);
    }
    if (sendAttempts >= 5)
    {
        ledSetColor(YELLOW);
    }
    else
    {
        ledSetColor(BLUE);
    }
}