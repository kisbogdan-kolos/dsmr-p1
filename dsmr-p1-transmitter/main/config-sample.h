#pragma once

#define UART_BUF_SIZE 1024 // bytes
#define UART_PORT_NUM UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_RX_PIN 3
#define UART_RX_INVERT

#define TELEGRAM_MAX_LENGTH 4096 // bytes

#define DATA_QUEUE_LENGTH 150         // items
#define DATA_QUEUE_SAVE_LIMIT 110     // items
#define DATA_QUEUE_SAVE_SIZE 100      // items
#define DATA_QUEUE_READ_SIZE 140      // items
#define DATA_QUEUE_SAVE_TIME (60 * 5) // s

#define ESPNOW_CHANNEL 11
#define ESPNOW_DEST_MAC                    \
    {                                      \
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab \
    }
#define ESPNOW_PMK_KEY "aaaaaaaaaaaaaaa"
#define ESPNOW_LMK_KEY "bbbbbbbbbbbbbbb"
#define ESPNOW_ENCRYPT true
#define ESPNOW_SLOW_SEND_LIMIT 10 // attempts

#define LED_PIN 8
#define LIGHT_SENSOR ADC_CHANNEL_2
#define BRIGHTNESS_ADJUST_INTERVAL (60) // s

// #define LEAK_DETECTOR
