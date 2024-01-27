#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

void datastoreInit();

void datastoreSave(QueueHandle_t queue, size_t count);

void datastoreRead(QueueHandle_t queue, size_t maxCount);
