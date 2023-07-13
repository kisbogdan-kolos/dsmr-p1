#pragma once

#define WIFI_SSID "kisbogdan-not-u"
#define WIFI_PASS "484QN4CHGF6CJ2UV"
#define WIFI_AUTHMODE WIFI_AUTH_WPA2_PSK

#define ESPNOW_CHANNEL 11
#define ESPNOW_DEST_MAC                    \
    {                                      \
        0xf4, 0x12, 0xfa, 0x18, 0x73, 0xac \
    }
#define ESPNOW_PMK_KEY "aaaaaaaaaaaaaaa"
#define ESPNOW_LMK_KEY "bbbbbbbbbbbbbbb"
#define ESPNOW_ENCRYPT true

#define WEBSOCKET_URI "ws://esp.kisbogdan.hu"
#define WEBSOCKET_PORT 80
#define WEBSOCKET_PATH "/power-meter"
#define WEBSOCKET_RECONNECT_TIMEOUT 10000
#define WEBSOCKET_NETWORK_TIMEOUT 10000

#define LED_PIN 8
#define LIGHT_SENSOR ADC_CHANNEL_2
#define BRIGHTNESS_ADJUST_INTERVAL (60)

// #define LEAK_DETECTOR
