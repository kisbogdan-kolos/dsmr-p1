#pragma once

#include "freertos/semphr.h"

#include "telegram/telegram.h"

void receiveInit(SemaphoreHandle_t wifiGotIp);

void receiveAck(uint32_t id);
