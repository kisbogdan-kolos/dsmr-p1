#pragma once

#define WIFI_SSID "wifi-ssid"
#define WIFI_PASS "wifi-password"
#define WIFI_AUTHMODE WIFI_AUTH_WPA2_PSK

#define ESPNOW_CHANNEL 11
#define ESPNOW_DEST_MAC                    \
    {                                      \
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab \
    }
#define ESPNOW_PMK_KEY "aaaaaaaaaaaaaaa" // 15 characters + '\0'
#define ESPNOW_LMK_KEY "bbbbbbbbbbbbbbb" // 15 characters + '\0'
#define ESPNOW_ENCRYPT true

#define WEBSOCKET_URI "ws://esp.example.com"
#define WEBSOCKET_PORT 80
#define WEBSOCKET_PATH "/power-meter"
#define WEBSOCKET_RECONNECT_TIMEOUT 10000 // ms
#define WEBSOCKET_NETWORK_TIMEOUT 10000   // ms

#define LED_PIN 8
#define LED_TWO_LEDS
#define LED_INACTIVE_TIME 30000 // ms
#define LIGHT_SENSOR ADC_CHANNEL_2
#define BRIGHTNESS_ADJUST_INTERVAL (60) // s

// #define LEAK_DETECTOR

// #define CPU_MONITOR // must enable 'configGENERATE_RUN_TIME_STATS' in 'sdkconfig
