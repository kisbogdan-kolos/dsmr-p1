#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "string.h"

#include "config.h"
#include "utils.h"

#include "uart.h"
#include "espnow/send.h"
#include "telegram/processData.h"
#include "led/led.h"

static const char *TAG = "uart";

static void dsmrReadTask(void *arg)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_PIN_NO_CHANGE, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

#ifdef UART_RX_INVERT
    ESP_ERROR_CHECK(uart_set_line_inverse(UART_PORT_NUM, UART_SIGNAL_RXD_INV));
#endif

    char *data = (char *)malloc(UART_BUF_SIZE);
    char *telegram = (char *)malloc(TELEGRAM_MAX_LENGTH);
    size_t idx = 0;
    bool endOfData = false;

#ifdef LED_TWO_LEDS
    ledSetColor(GREEN, 1);
#endif

    while (1)
    {
        int len = uart_read_bytes(UART_PORT_NUM, data, (UART_BUF_SIZE - 1), 200 / portTICK_PERIOD_MS);

        if (len > 0)
        {
            data[len] = 0;
            if (idx == 0)
            {
                char *start = strchr(data, '/');
                if (start != NULL)
                {
                    size_t dataLen = strlen(start);
                    strcpy(telegram, start);
                    idx = dataLen;
#ifdef LED_TWO_LEDS
                    ledSetColor(CYAN, 1);
#else
                    ledSetColor(CYAN, 0);
#endif
                }
                else
                {
                    ESP_LOGW(TAG, "No start tag received");
                }
            }
            else
            {
                char *end = strchr(data, '!');
                size_t maxLen = TELEGRAM_MAX_LENGTH - idx - 1;
                if (end == NULL)
                {
                    strncpy(telegram + idx, data, maxLen);
                    idx += min(len, maxLen);
                }
                else
                {
                    strncpy(telegram + idx, data, min(maxLen, end - data + 1));
                    idx += min(maxLen, end - data + 1);
                    endOfData = true;
                }
                telegram[idx] = 0;
                if (idx == TELEGRAM_MAX_LENGTH - 1)
                {
                    ESP_LOGE(TAG, "Buffer full, and no end received.");
                    idx = 0;
                    endOfData = false;
                }
                else if (endOfData)
                {
                    idx = 0;
                    endOfData = false;
                    Data *data = processData(telegram);
                    if (data != NULL)
                    {
                        sendQueueData(data);
                    }
                }
            }
        }
    }
}

void uartInit()
{
#ifdef LED_TWO_LEDS
    ledSetColor(YELLOW, 1);
#else
    ledSetColor(YELLOW, 0);
#endif
    xTaskCreate(dsmrReadTask, "uart_task", 4096, NULL, 10, NULL);

#ifndef LED_TWO_LEDS
    vTaskDelay(1200 / portTICK_PERIOD_MS);
#endif
}
