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

#include "receive.h"
#include "espnow-dsmr.h"
#include "wifi.h"
#include "websocket/websocket.h"
#include "led/led.h"

static const char *TAG = "espnow-recv";

static QueueHandle_t s_example_espnow_queue;
static QueueHandle_t dataQueueHandle;

#ifdef LED_TWO_LEDS
static SemaphoreHandle_t gotData;
#endif

static uint8_t destMac[6] = ESPNOW_DEST_MAC;

static void dsmr_wifi_init(SemaphoreHandle_t wifiGotIp)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifiInit(wifiGotIp);
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_LR | WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));
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
        if (uxQueueMessagesWaiting(s_example_espnow_queue) == 0 && uxQueueMessagesWaiting(dataQueueHandle) > 0)
        {
            evt.id = EXAMPLE_ESPNOW_SEND_CB;
            xQueueSend(s_example_espnow_queue, &evt, 0);
        }

        if (xQueueReceive(s_example_espnow_queue, &evt, 1000 / portTICK_PERIOD_MS) == pdTRUE)
        {
            switch (evt.id)
            {
            case EXAMPLE_ESPNOW_SEND_CB:
            {
                uint32_t id;

                if (xQueueReceive(dataQueueHandle, &id, 0) == pdTRUE)
                {
                    dsmr_espnow_payload_send *payload = (dsmr_espnow_payload_send *)malloc(sizeof(dsmr_espnow_payload_send));
                    payload->id = id;
                    payload->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)&(payload->id), sizeof(uint32_t));

                    if (esp_now_send(destMac, (const uint8_t *)payload, sizeof(dsmr_espnow_payload_send)) != ESP_OK)
                    {
                        ESP_LOGE(TAG, "Send error, rebooting...");
                        ledSetColor(RED, 0);
                        vTaskDelay(10000 / portTICK_PERIOD_MS);
                        esp_restart();
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Send data id: %lu", payload->id);
                    }

                    free(payload);
                }

                vTaskDelay(100 / portTICK_PERIOD_MS);

                ledSetColor(GREEN, 0);

                break;
            }
            case EXAMPLE_ESPNOW_RECV_CB:
            {
                dsmr_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

                if (recv_cb->data_len == sizeof(dsmr_espnow_payload_recv))
                {
                    dsmr_espnow_payload_recv *payload = (dsmr_espnow_payload_recv *)recv_cb->data;
                    uint16_t calc_crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)&(payload->data), sizeof(Data));
                    if (payload->crc == calc_crc)
                    {
                        ESP_LOGI(TAG, "Received data id: %lu", payload->data.id);

                        if (wifiIsConnected() && websocketIsConnected())
                            ledSetColor(BLUE, 0);

#ifdef LED_TWO_LEDS
                        xSemaphoreGive(gotData);
#endif

                        websocketSendData(&(payload->data));
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

    xTaskCreate(dsmr_espnow_task, "espnow", 2048, NULL, 4, NULL);

    return ESP_OK;
}

#ifdef LED_TWO_LEDS
static void ledTask(void *pvParameter)
{
    ledSetColor(RED, 1);
    for (;;)
    {
        if (xSemaphoreTake(gotData, LED_INACTIVE_TIME / portTICK_PERIOD_MS) == pdTRUE)
        {
            ledSetColor(GREEN, 1);
        }
        else
        {
            ledSetColor(MAGENTA, 1);
        }
    }
}
#endif

void receiveInit(SemaphoreHandle_t wifiGotIp)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    dataQueueHandle = xQueueCreate(150, sizeof(uint32_t));
    if (dataQueueHandle == NULL)
    {
        ESP_LOGE(TAG, "Create data queue fail");
    }

#ifdef LED_TWO_LEDS
    gotData = xSemaphoreCreateBinary();
    if (gotData == NULL)
    {
        ESP_LOGE(TAG, "Create gotData semaphore fail");
    }
    xTaskCreate(ledTask, "led_inactive", 2048, NULL, 4, NULL);
#endif

    dsmr_wifi_init(wifiGotIp);
    dsmr_espnow_init();

    ledSetColor(MAGENTA, 0);
}

void receiveAck(uint32_t id)
{
    if (xQueueSend(dataQueueHandle, &id, 0) != pdTRUE)
    {
        ESP_LOGE(TAG, "Send data to queue fail");
    }
    else if (uxQueueMessagesWaiting(s_example_espnow_queue) == 0)
    {
        dsmr_espnow_event_t evt;
        evt.id = EXAMPLE_ESPNOW_SEND_CB;
        xQueueSend(s_example_espnow_queue, &evt, 0);
    }
}
