#include "processData.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "telegram.h"
#include "esp_random.h"

static const char *TAG = "process-data";

static void cleanData(const char *input, char *output)
{
    char lastChar = 0;
    while (*input != 0)
    {
        if (*input == '\r' || *input == '\n')
        {
            if (lastChar != '\n')
                *output++ = '\n';
        }
        else
            *output++ = *input;
        input++;
        lastChar = *(output - 1);
    }
    *output = 0;
}

static void trimData(char *data)
{
    size_t len = strlen(data);
    if (len < 5)
    {
        ESP_LOGE(TAG, "Trim: Data length too small");
        return;
    }
    if (data[0] == '/' && data[4] == '5')
    {
        char *start = strchr(data, '\n');
        if (start != NULL)
        {
            // strcpy is UB if strings overlap
            char *from = start + 1;
            char *to = data;
            while (*from != 0)
            {
                *to++ = *from++;
            }
            *to = 0;
        }
        else
        {
            ESP_LOGE(TAG, "Trim: no '\\n' found in string");
        }
    }
    char *end = strrchr(data, '\n');
    if (end != NULL)
    {
        *end = 0;
    }
    else
    {
        ESP_LOGE(TAG, "Trim: no last '\\n' found in string");
    }
}

static void processTime(const char *data, Time *time, Time *processed)
{
    if (sscanf(data, "%2u%2u%2u%2u%2u%2us", &(time->year), &(time->month), &(time->day), &(time->hour), &(time->minute), &(time->second)) == 6)
    {
        processed->day = 1;
        processed->hour = 1;
        processed->minute = 1;
        processed->month = 1;
        processed->second = 1;
        processed->year = 1;

        time->year += 2000;
    }
    else
    {
        ESP_LOGE(TAG, "Time: unable to parse time");
    }
}

static void removeDot(char *data)
{
    char *dot = strchr(data, '.');
    if (dot != NULL)
    {
        char *from = dot + 1;
        char *to = dot;
        while (*from != 0)
        {
            *to++ = *from++;
        }
        *to = 0;
    }
    else
    {
        ESP_LOGE(TAG, "Remove dot: no dot found");
    }
}

static void processSegment(char *segment, Data *parsedData, Data *processed)
{
    char *dataStart = strchr(segment, '(');
    char *dataEnd = segment + strlen(segment) - 1;
    if (dataStart == NULL)
    {
        ESP_LOGE(TAG, "Segment: unable to find data start");
        return;
    }

    *dataStart = 0;
    *dataEnd = 0;

    char *obis = segment;
    char *data = dataStart + 1;

    if (strcmp(obis, "0-0:1.0.0") == 0)
    {
        processTime(data, &(parsedData->time), &(processed->time));
    }
    else if (strcmp(obis, "0-0:96.1.0") == 0)
    {
        if (sscanf(data, "%llu", &(parsedData->meterNumber)) == 1)
        {
            processed->meterNumber = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Meter number: unable to parse meter number");
        }
    }
    else if (strcmp(obis, "0-0:96.14.0") == 0)
    {
        if (sscanf(data, "%hhu", &(parsedData->currentTariff)) == 1)
        {
            processed->currentTariff = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Current tariff: unable to parse current tariff");
        }
    }
    else if (strcmp(obis, "1-0:1.8.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kWh", &(parsedData->all.importedEnergy)) == 1)
        {
            processed->all.importedEnergy = 1;
        }
        else
        {
            ESP_LOGE(TAG, "All imported energy: unable to parse all imported energy");
        }
    }
    else if (strcmp(obis, "1-0:1.8.1") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kWh", &(parsedData->tariff1.importedEnergy)) == 1)
        {
            processed->tariff1.importedEnergy = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Tariff 1 imported energy: unable to parse tariff 1 imported energy");
        }
    }
    else if (strcmp(obis, "1-0:1.8.2") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kWh", &(parsedData->tariff2.importedEnergy)) == 1)
        {
            processed->tariff2.importedEnergy = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Tariff 2 imported energy: unable to parse tariff 2 imported energy");
        }
    }
    else if (strcmp(obis, "1-0:2.8.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kWh", &(parsedData->all.exportedEnergy)) == 1)
        {
            processed->all.exportedEnergy = 1;
        }
        else
        {
            ESP_LOGE(TAG, "All exported energy: unable to parse all exported energy");
        }
    }
    else if (strcmp(obis, "1-0:2.8.1") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kWh", &(parsedData->tariff1.exportedEnergy)) == 1)
        {
            processed->tariff1.exportedEnergy = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Tariff 1 exported energy: unable to parse tariff 1 exported energy");
        }
    }
    else if (strcmp(obis, "1-0:2.8.2") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kWh", &(parsedData->tariff2.exportedEnergy)) == 1)
        {
            processed->tariff2.exportedEnergy = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Tariff 2 exported energy: unable to parse tariff 2 exported energy");
        }
    }
    else if (strcmp(obis, "1-0:3.8.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kvarh", &(parsedData->importreactiveEnergy)) == 1)
        {
            processed->importreactiveEnergy = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Import reactive energy: unable to parse import reactive energy");
        }
    }
    else if (strcmp(obis, "1-0:4.8.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kvarh", &(parsedData->exportreactiveEnergy)) == 1)
        {
            processed->exportreactiveEnergy = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Export reactive energy: unable to parse export reactive energy");
        }
    }
    else if (strcmp(obis, "1-0:5.8.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kvarh", &(parsedData->reactiveEnergy[0])) == 1)
        {
            processed->reactiveEnergy[0] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "reactive energy QI: unable to parse reactive energy QI");
        }
    }
    else if (strcmp(obis, "1-0:6.8.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kvarh", &(parsedData->reactiveEnergy[1])) == 1)
        {
            processed->reactiveEnergy[1] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "reactive energy QII: unable to parse reactive energy QII");
        }
    }
    else if (strcmp(obis, "1-0:7.8.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kvarh", &(parsedData->reactiveEnergy[2])) == 1)
        {
            processed->reactiveEnergy[2] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "reactive energy QIII: unable to parse reactive energy QIII");
        }
    }
    else if (strcmp(obis, "1-0:8.8.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kvarh", &(parsedData->reactiveEnergy[3])) == 1)
        {
            processed->reactiveEnergy[3] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "reactive energy QIV: unable to parse reactive energy QIV");
        }
    }
    else if (strcmp(obis, "1-0:32.7.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%hu*V", &(parsedData->voltage[0])) == 1)
        {
            processed->voltage[0] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Voltage L1: unable to parse voltage L1");
        }
    }
    else if (strcmp(obis, "1-0:52.7.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%hu*V", &(parsedData->voltage[1])) == 1)
        {
            processed->voltage[1] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Voltage L2: unable to parse voltage L2");
        }
    }
    else if (strcmp(obis, "1-0:72.7.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%hu*V", &(parsedData->voltage[2])) == 1)
        {
            processed->voltage[2] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Voltage L3: unable to parse voltage L3");
        }
    }
    else if (strcmp(obis, "1-0:31.7.0") == 0)
    {
        if (sscanf(data, "%hhu*A", &(parsedData->current[0])) == 1)
        {
            processed->current[0] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Current L1: unable to parse current L1");
        }
    }
    else if (strcmp(obis, "1-0:51.7.0") == 0)
    {
        if (sscanf(data, "%hhu*A", &(parsedData->current[1])) == 1)
        {
            processed->current[1] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Current L2: unable to parse current L2");
        }
    }
    else if (strcmp(obis, "1-0:71.7.0") == 0)
    {
        if (sscanf(data, "%hhu*A", &(parsedData->current[2])) == 1)
        {
            processed->current[2] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Current L3: unable to parse current L3");
        }
    }
    else if (strcmp(obis, "1-0:13.7.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu", &(parsedData->combinedPowerFactor)) == 1)
        {
            processed->combinedPowerFactor = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Combined power factor: unable to parse combined power factor");
        }
    }
    else if (strcmp(obis, "1-0:33.7.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu", &(parsedData->powerFactor[0])) == 1)
        {
            processed->powerFactor[0] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Power factor L1: unable to parse power factor L1");
        }
    }
    else if (strcmp(obis, "1-0:53.7.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu", &(parsedData->powerFactor[1])) == 1)
        {
            processed->powerFactor[1] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Power factor L2: unable to parse power factor L2");
        }
    }
    else if (strcmp(obis, "1-0:73.7.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu", &(parsedData->powerFactor[2])) == 1)
        {
            processed->powerFactor[2] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Power factor L3: unable to parse power factor L3");
        }
    }
    else if (strcmp(obis, "1-0:14.7.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*Hz", &(parsedData->frequency)) == 1)
        {
            processed->frequency = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Frequency: unable to parse frequency");
        }
    }
    else if (strcmp(obis, "1-0:1.7.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kW", &(parsedData->importPower)) == 1)
        {
            processed->importPower = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Import power: unable to parse import power");
        }
    }
    else if (strcmp(obis, "1-0:2.7.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kW", &(parsedData->exportPower)) == 1)
        {
            processed->exportPower = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Export power: unable to parse export power");
        }
    }
    else if (strcmp(obis, "1-0:5.7.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kvar", &(parsedData->reactivePower[0])) == 1)
        {
            processed->reactivePower[0] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "reactive power QI: unable to parse reactive power QI");
        }
    }
    else if (strcmp(obis, "1-0:6.7.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kvar", &(parsedData->reactivePower[1])) == 1)
        {
            processed->reactivePower[1] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "reactive power QII: unable to parse reactive power QII");
        }
    }
    else if (strcmp(obis, "1-0:7.7.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kvar", &(parsedData->reactivePower[2])) == 1)
        {
            processed->reactivePower[2] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "reactive power QIII: unable to parse reactive power QIII");
        }
    }
    else if (strcmp(obis, "1-0:8.7.0") == 0)
    {
        removeDot(data);
        if (sscanf(data, "%lu*kvar", &(parsedData->reactivePower[3])) == 1)
        {
            processed->reactivePower[3] = 1;
        }
        else
        {
            ESP_LOGE(TAG, "reactive power QIV: unable to parse reactive power QIV");
        }
    }
}

Data *processData(const char *telegram)
{
    char *cleanedData = (char *)malloc(strlen(telegram));
    cleanData(telegram, cleanedData);
    trimData(cleanedData);

    Data *data = malloc(sizeof(Data));
    Data *processed = malloc(sizeof(Data));
    telegramZeroFill(processed);

    char *segmentStart = cleanedData;
    char *segmentEnd = strchr(segmentStart, '\n');
    while (segmentEnd != NULL)
    {
        *segmentEnd = 0;
        processSegment(segmentStart, data, processed);
        segmentStart = segmentEnd + 1;
        segmentEnd = strchr(segmentStart, '\n');
    }

    free(cleanedData);

    bool isComplete = telegramIsPopulated(processed);

    if (isComplete)
    {
        free(processed);
        data->id = esp_random();
        return data;
    }
    else
    {
        free(processed);
        free(data);
        ESP_LOGE(TAG, "Data is not complete");
        return NULL;
    }
}