#pragma once

#include <stdio.h>
#include <stdbool.h>

typedef struct Time
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} Time;

typedef struct Tariff
{
    uint32_t importedEnergy;
    uint32_t exportedEnergy;
} Tariff;

typedef struct Data
{
    Time time;
    uint64_t meterNumber;
    uint8_t currentTariff;
    Tariff all;
    Tariff tariff1;
    Tariff tariff2;
    uint32_t importreactiveEnergy;
    uint32_t exportreactiveEnergy;
    uint32_t reactiveEnergy[4];
    uint16_t voltage[3];
    uint8_t current[3];
    uint16_t combinedPowerFactor;
    uint16_t powerFactor[3];
    uint16_t frequency;
    uint16_t importPower;
    uint16_t exportPower;
    uint16_t reactivePower[4];

    uint32_t id;
} Data;

void telegramPrint(Data *data);
void telegramZeroFill(Data *data);
bool telegramIsPopulated(Data *data);
