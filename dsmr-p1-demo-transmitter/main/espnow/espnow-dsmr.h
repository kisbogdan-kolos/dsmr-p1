#pragma once

#include <stdio.h>
#include "esp_now.h"

#include "telegram/telegram.h"

typedef enum
{
    EXAMPLE_ESPNOW_SEND_CB,
    EXAMPLE_ESPNOW_RECV_CB,
} dsmr_espnow_event_id_t;

typedef struct
{
    uint8_t mac_addr[6];
    esp_now_send_status_t status;
} dsmr_espnow_event_send_cb_t;

typedef struct
{
    uint8_t mac_addr[6];
    uint8_t *data;
    int data_len;
} dsmr_espnow_event_recv_cb_t;

typedef union
{
    dsmr_espnow_event_send_cb_t send_cb;
    dsmr_espnow_event_recv_cb_t recv_cb;
} dsmr_espnow_event_info_t;

typedef struct
{
    dsmr_espnow_event_id_t id;
    dsmr_espnow_event_info_t info;
} dsmr_espnow_event_t;

typedef struct
{
    Data data;
    uint16_t crc;
} dsmr_espnow_payload_send;

typedef struct
{
    uint32_t id;
    uint16_t crc;
} dsmr_espnow_payload_recv;
