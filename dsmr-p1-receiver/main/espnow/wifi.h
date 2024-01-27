#pragma once

#include "freertos/semphr.h"

void wifiInit(SemaphoreHandle_t wifiGotIp);

void wifiDisconnect();

void wifiReconnect();

bool wifiIsConnected();
