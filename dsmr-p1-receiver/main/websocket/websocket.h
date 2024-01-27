#pragma once

#include "telegram/telegram.h"

void websocketInit();

void websocketDestroy();

void websocketSendData(Data *data);

bool websocketIsConnected();
