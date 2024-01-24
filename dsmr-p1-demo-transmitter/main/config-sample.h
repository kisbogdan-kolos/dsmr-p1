#pragma once

#define DEMO_SEND_INTERVAL 10000 // ms

#define ESPNOW_CHANNEL 11
#define ESPNOW_DEST_MAC                    \
    {                                      \
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab \
    }
#define ESPNOW_PMK_KEY "aaaaaaaaaaaaaaa"
#define ESPNOW_LMK_KEY "bbbbbbbbbbbbbbb"
#define ESPNOW_ENCRYPT true

#define LED_PIN 8
#define LIGHT_SENSOR ADC_CHANNEL_2
#define BRIGHTNESS_ADJUST_INTERVAL (60) // s

// #define LEAK_DETECTOR
