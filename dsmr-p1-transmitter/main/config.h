#pragma once

#define UART_BUF_SIZE 1024
#define UART_PORT_NUM UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_RX_PIN 3
#define UART_RX_INVERT

#define TELEGRAM_MAX_LENGTH 4096

#define DATA_QUEUE_LENGTH 150
#define DATA_QUEUE_SAVE_LIMIT 11
#define DATA_QUEUE_SAVE_SIZE 10
#define DATA_QUEUE_READ_SIZE 140
#define DATA_QUEUE_SAVE_TIME (60 * 5)

#define ESPNOW_CHANNEL 11
#define ESPNOW_DEST_MAC                    \
    {                                      \
        0xf4, 0x12, 0xfa, 0x15, 0xc8, 0x94 \
    }
#define ESPNOW_PMK_KEY "aaaaaaaaaaaaaaa"
#define ESPNOW_LMK_KEY "bbbbbbbbbbbbbbb"
#define ESPNOW_ENCRYPT true

#define LED_PIN 8
#define LIGHT_SENSOR ADC_CHANNEL_2
#define BRIGHTNESS_ADJUST_INTERVAL (60)

// #define LEAK_DETECTOR
