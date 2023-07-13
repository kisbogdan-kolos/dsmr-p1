#include "telegram.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"

static const char *TAG = "telegram";

void telegramPrint(Data *telegram)
{
    ESP_LOGI(TAG, "Telegram:");
    ESP_LOGI(TAG, "  Time: %04d-%02d-%02d %02d:%02d:%02d",
             telegram->time.year,
             telegram->time.month,
             telegram->time.day,
             telegram->time.hour,
             telegram->time.minute,
             telegram->time.second);
    ESP_LOGI(TAG, "  Meter number: %llu", telegram->meterNumber);
    ESP_LOGI(TAG, "  Current tariff: %hhu", telegram->currentTariff);
    ESP_LOGI(TAG, "  All:");
    ESP_LOGI(TAG, "    Imported energy: %lu", telegram->all.importedEnergy);
    ESP_LOGI(TAG, "    Exported energy: %lu", telegram->all.exportedEnergy);
    ESP_LOGI(TAG, "  Tariff 1:");
    ESP_LOGI(TAG, "    Imported energy: %lu", telegram->tariff1.importedEnergy);
    ESP_LOGI(TAG, "    Exported energy: %lu", telegram->tariff1.exportedEnergy);
    ESP_LOGI(TAG, "  Tariff 2:");
    ESP_LOGI(TAG, "    Imported energy: %lu", telegram->tariff2.importedEnergy);
    ESP_LOGI(TAG, "    Exported energy: %lu", telegram->tariff2.exportedEnergy);
    ESP_LOGI(TAG, "  Import reactive energy: %lu", telegram->importreactiveEnergy);
    ESP_LOGI(TAG, "  Export reactive energy: %lu", telegram->exportreactiveEnergy);
    ESP_LOGI(TAG, "  reactive energy:");
    ESP_LOGI(TAG, "    QI:   %lu", telegram->reactiveEnergy[0]);
    ESP_LOGI(TAG, "    QII:  %lu", telegram->reactiveEnergy[1]);
    ESP_LOGI(TAG, "    QIII: %lu", telegram->reactiveEnergy[2]);
    ESP_LOGI(TAG, "    QIV:  %lu", telegram->reactiveEnergy[3]);
    ESP_LOGI(TAG, "  Voltage:");
    ESP_LOGI(TAG, "    L1: %hu", telegram->voltage[0]);
    ESP_LOGI(TAG, "    L2: %hu", telegram->voltage[1]);
    ESP_LOGI(TAG, "    L3: %hu", telegram->voltage[2]);
    ESP_LOGI(TAG, "  Current:");
    ESP_LOGI(TAG, "    L1: %hhu", telegram->current[0]);
    ESP_LOGI(TAG, "    L2: %hhu", telegram->current[1]);
    ESP_LOGI(TAG, "    L3: %hhu", telegram->current[2]);
    ESP_LOGI(TAG, "  Combined power factor: %hu", telegram->combinedPowerFactor);
    ESP_LOGI(TAG, "  Power factor:");
    ESP_LOGI(TAG, "    L1: %hu", telegram->powerFactor[0]);
    ESP_LOGI(TAG, "    L2: %hu", telegram->powerFactor[1]);
    ESP_LOGI(TAG, "    L3: %hu", telegram->powerFactor[2]);
    ESP_LOGI(TAG, "  Frequency: %hu", telegram->frequency);
    ESP_LOGI(TAG, "  Import power: %hu", telegram->importPower);
    ESP_LOGI(TAG, "  Export power: %hu", telegram->exportPower);
    ESP_LOGI(TAG, "  reactive power:");
    ESP_LOGI(TAG, "    QI:   %hu", telegram->reactivePower[0]);
    ESP_LOGI(TAG, "    QII:  %hu", telegram->reactivePower[1]);
    ESP_LOGI(TAG, "    QIII: %hu", telegram->reactivePower[2]);
    ESP_LOGI(TAG, "    QIV:  %hu", telegram->reactivePower[3]);
}

void telegramZeroFill(Data *telegram)
{
    memset(telegram, 0, sizeof(Data));
}

bool telegramIsPopulated(Data *telegram)
{
    return telegram->time.year != 0 &&
           telegram->time.month != 0 &&
           telegram->time.day != 0 &&
           telegram->time.hour != 0 &&
           telegram->time.minute != 0 &&
           telegram->time.second != 0 &&
           telegram->meterNumber != 0 &&
           telegram->currentTariff != 0 &&
           telegram->all.importedEnergy != 0 &&
           telegram->all.exportedEnergy != 0 &&
           telegram->tariff1.importedEnergy != 0 &&
           telegram->tariff1.exportedEnergy != 0 &&
           telegram->tariff2.importedEnergy != 0 &&
           telegram->tariff2.exportedEnergy != 0 &&
           telegram->importreactiveEnergy != 0 &&
           telegram->exportreactiveEnergy != 0 &&
           telegram->reactiveEnergy[0] != 0 &&
           telegram->reactiveEnergy[1] != 0 &&
           telegram->reactiveEnergy[2] != 0 &&
           telegram->reactiveEnergy[3] != 0 &&
           telegram->voltage[0] != 0 &&
           telegram->voltage[1] != 0 &&
           telegram->voltage[2] != 0 &&
           telegram->current[0] != 0 &&
           telegram->current[1] != 0 &&
           telegram->current[2] != 0 &&
           telegram->combinedPowerFactor != 0 &&
           telegram->powerFactor[0] != 0 &&
           telegram->powerFactor[1] != 0 &&
           telegram->powerFactor[2] != 0 &&
           telegram->frequency != 0 &&
           telegram->importPower != 0 &&
           telegram->exportPower != 0 &&
           telegram->reactivePower[0] != 0 &&
           telegram->reactivePower[1] != 0 &&
           telegram->reactivePower[2] != 0 &&
           telegram->reactivePower[3] != 0;
}