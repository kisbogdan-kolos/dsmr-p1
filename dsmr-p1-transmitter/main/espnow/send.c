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

static SemaphoreHandle_t dataSent;
static SemaphoreHandle_t dataAcked;
static SemaphoreHandle_t currentDataUsed;

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
    if (mac_addr == NULL)
    {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    if (status == ESP_NOW_SEND_SUCCESS)
    {
        xSemaphoreGive(dataSent);
    }
}

static void dsmr_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    dsmr_espnow_event_recv_cb_t recv_cb;
    uint8_t *mac_addr = recv_info->src_addr;

    if (mac_addr == NULL || data == NULL || len <= 0)
    {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    memcpy(recv_cb.mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv_cb.data = malloc(len);
    if (recv_cb.data == NULL)
    {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb.data, data, len);
    recv_cb.data_len = len;
    if (xQueueGenericSend(s_example_espnow_queue, &recv_cb, 0, queueSEND_TO_FRONT) != pdTRUE)
    {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(recv_cb.data);
    }
}

static void espnowSendTask(void *pvParameter)
{
    for (;;)
    {
        while (currentData == NULL)
            xQueueReceive(dataQueueHandle, &currentData, portMAX_DELAY);

        while (xSemaphoreTake(currentDataUsed, portMAX_DELAY) != pdTRUE)
            ;

        if (currentData != NULL)
        {
            dsmr_espnow_payload_send *payload = (dsmr_espnow_payload_send *)malloc(sizeof(dsmr_espnow_payload_send));
            memcpy(&(payload->data), currentData, sizeof(Data));
            payload->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)&(payload->data), sizeof(Data));

            xSemaphoreGive(currentDataUsed);

            xSemaphoreTake(dataSent, 100 / portTICK_PERIOD_MS);
            if (esp_now_send(destMac, (const uint8_t *)payload, sizeof(dsmr_espnow_payload_send)) != ESP_OK)
            {
                ESP_LOGE(TAG, "Send error, rebooting...");
                ledSetColor(RED, 0);

                while (xSemaphoreTake(currentDataUsed, portMAX_DELAY) != pdTRUE)
                    ;
                if (currentData != NULL)
                    xQueueSend(dataQueueHandle, &currentData, 0);
                currentData = NULL;
                xSemaphoreGive(currentDataUsed);

                datastoreSave(dataQueueHandle, uxQueueMessagesWaiting(dataQueueHandle));

                vTaskDelay(10000 / portTICK_PERIOD_MS);
                esp_restart();
            }

            ESP_LOGI(TAG, "Send data id: %lu", payload->data.id);
            sendAttempts++;

            free(payload);

            if (sendAttempts >= ESPNOW_SLOW_SEND_LIMIT)
            {
#ifdef LED_TWO_LEDS
                ledSetColor(YELLOW, 1);
#else
                ledSetColor(YELLOW, 0);
#endif
                ESP_LOGW(TAG, "Slow send limit reached, increasing wait time");
                xSemaphoreTake(dataAcked, ESPNOW_SLOW_RESEND_INTERVAL / portTICK_PERIOD_MS);
            }
            else
            {
#ifdef LED_TWO_LEDS
                ledSetColor(BLUE, 1);
#else
                ledSetColor(BLUE, 0);
#endif
                xSemaphoreTake(dataAcked, ESPNOW_RESEND_INTERVAL / portTICK_PERIOD_MS);
            }
        }
        else
        {
            xSemaphoreGive(currentDataUsed);
        }
    }
}

static void espnowReceiveTask(void *pvParameter)
{
    dsmr_espnow_event_recv_cb_t recv_cb;

    for (;;)
    {
        if (xQueueReceive(s_example_espnow_queue, &recv_cb, portMAX_DELAY) == pdTRUE)
        {
            if (recv_cb.data_len == sizeof(dsmr_espnow_payload_recv))
            {
                dsmr_espnow_payload_recv *payload = (dsmr_espnow_payload_recv *)recv_cb.data;
                uint16_t calc_crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)&(payload->id), sizeof(uint32_t));
                if (payload->crc == calc_crc)
                {
                    while (xSemaphoreTake(currentDataUsed, portMAX_DELAY) != pdTRUE)
                        ;

                    if (currentData != NULL && currentData->id == payload->id)
                    {
                        ESP_LOGI(TAG, "Received data id: %lu", payload->id);
                        free(currentData);
                        currentData = NULL;

                        xSemaphoreGive(dataAcked);

#ifdef LED_TWO_LEDS
                        ledSetColor(GREEN, 1);
#else
                        ledSetColor(GREEN, 0);
#endif

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
                    xSemaphoreGive(currentDataUsed);
                }
                else
                {
                    ESP_LOGE(TAG, "CRC mismatch");
                }
            }
            else
            {
                ESP_LOGE(TAG, "Received data with wrong size, expected %u, got %d", sizeof(dsmr_espnow_payload_recv), recv_cb.data_len);
            }

            free(recv_cb.data);
        }
    }
}

static void dataSaveTask(void *pvParameter)
{
    for (;;)
    {
        vTaskDelay(DATA_QUEUE_SAVE_TIME * configTICK_RATE_HZ);

        uint32_t cnt = uxQueueMessagesWaiting(dataQueueHandle);
        if (cnt == 0)
        {
            datastoreRead(dataQueueHandle, DATA_QUEUE_READ_SIZE);
        }
    }
}

static esp_err_t dsmr_espnow_init(void)
{
    s_example_espnow_queue = xQueueCreate(10, sizeof(dsmr_espnow_event_recv_cb_t));
    if (s_example_espnow_queue == NULL)
    {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    dataSent = xSemaphoreCreateBinary();
    if (dataSent == NULL)
    {
        ESP_LOGE(TAG, "Create semaphore fail");
        vSemaphoreDelete(s_example_espnow_queue);
        return ESP_FAIL;
    }
    xSemaphoreGive(dataSent);

    dataAcked = xSemaphoreCreateBinary();
    if (dataAcked == NULL)
    {
        ESP_LOGE(TAG, "Create semaphore fail");
        vSemaphoreDelete(s_example_espnow_queue);
        vSemaphoreDelete(dataSent);
        return ESP_FAIL;
    }
    xSemaphoreTake(dataAcked, 0);

    currentDataUsed = xSemaphoreCreateBinary();
    if (currentDataUsed == NULL)
    {
        ESP_LOGE(TAG, "Create semaphore fail");
        vSemaphoreDelete(s_example_espnow_queue);
        vSemaphoreDelete(dataSent);
        vSemaphoreDelete(dataAcked);
        return ESP_FAIL;
    }
    xSemaphoreGive(currentDataUsed);

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(dsmr_espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(dsmr_espnow_recv_cb));
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)ESPNOW_PMK_KEY));

    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL)
    {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(s_example_espnow_queue);
        vSemaphoreDelete(dataSent);
        vSemaphoreDelete(dataAcked);
        vSemaphoreDelete(currentDataUsed);
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

    xTaskCreate(espnowReceiveTask, "espnow_receive", 2048, NULL, 4, NULL);
    xTaskCreate(espnowSendTask, "espnow_send", 2048, NULL, 3, NULL);
    xTaskCreate(dataSaveTask, "espnow_save", 2048, NULL, 1, NULL);

    return ESP_OK;
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

    ledSetColor(MAGENTA, 0);
}

void sendQueueData(Data *data)
{
    if (xQueueSend(dataQueueHandle, &data, 0) != pdTRUE)
    {
        ESP_LOGE(TAG, "Send data to queue fail");
        free(data);
    }
    if (sendAttempts >= ESPNOW_SLOW_SEND_LIMIT)
    {
#ifdef LED_TWO_LEDS
        ledSetColor(YELLOW, 1);
#else
        ledSetColor(YELLOW, 0);
#endif
    }
    else
    {
#ifdef LED_TWO_LEDS
        ledSetColor(BLUE, 1);
#else
        ledSetColor(BLUE, 0);
#endif
    }

    uint32_t cnt = uxQueueMessagesWaiting(dataQueueHandle);
    if (cnt >= DATA_QUEUE_SAVE_LIMIT)
    {
        datastoreSave(dataQueueHandle, DATA_QUEUE_SAVE_SIZE);
    }
}