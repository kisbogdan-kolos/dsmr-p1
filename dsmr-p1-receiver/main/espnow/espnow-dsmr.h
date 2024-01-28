#pragma once

#include <stdio.h>
#include "esp_now.h"

#include "telegram/telegram.h"

typedef struct
{
    uint8_t mac_addr[6];
    uint8_t *data;
    int data_len;
} dsmr_espnow_event_recv_cb_t;

typedef struct
{
    Data data;
    uint16_t crc;
} dsmr_espnow_payload_recv;

typedef struct
{
    uint32_t id;
    uint16_t crc;
} dsmr_espnow_payload_send;
