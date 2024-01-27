#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_random.h"

#include "datastore.h"
#include "telegram/telegram.h"
#include "led/led.h"

static const char *TAG = "datastore";

static bool spiffsMounted = false;
static esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 16,
    .format_if_mount_failed = true,
};

static void datastoreInitTask(void *pvParameters)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }

#ifdef LED_TWO_LEDS
        ledSetColor(RED, 0);
#endif

        vTaskDelete(NULL);

        return;
    }

    spiffsMounted = true;

    ESP_LOGI(TAG, "Performing SPIFFS_check().");
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));

        vTaskDelete(NULL);

        return;
    }
    else
    {
        ESP_LOGI(TAG, "SPIFFS_check() successful");
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);

#ifdef LED_TWO_LEDS
        ledSetColor(RED, 0);
#endif

        vTaskDelete(NULL);

        return;
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    ledSetColor(GREEN, 0);

    vTaskDelete(NULL);
}

void datastoreInit()
{
    xTaskCreate(datastoreInitTask, "datastore_init", 4096, NULL, 0, NULL);
}

void datastoreSave(QueueHandle_t queue, size_t count)
{
    if (!spiffsMounted)
    {
        ESP_LOGE(TAG, "SPIFFS not mounted");
        return;
    }

#ifdef LED_TWO_LEDS
    ledSetColor(YELLOW, 0);
#endif

    size_t total = 0, used = 0;
    esp_err_t ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);

#ifdef LED_TWO_LEDS
        ledSetColor(RED, 0);
#endif

        return;
    }

    if (3 * used > 4 * total)
    {
        ESP_LOGE(TAG, "SPIFFS partition is more than 75%% full. Discarding all data.");

        Data *data;
        while (xQueueReceive(queue, &data, 0) == pdTRUE)
        {
            free(data);
        }

#ifdef LED_TWO_LEDS
        ledSetColor(RED, 0);
#endif

        return;
    }

    ESP_LOGI(TAG, "Saving %d items to SPIFFS", count);

    char *fname = malloc(32);
    sprintf(fname, "/spiffs/data_%d.bin", esp_random() % 1000);

    FILE *f = fopen(fname, "wb");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing");
        free(fname);

#ifdef LED_TWO_LEDS
        ledSetColor(RED, 0);
#endif

        return;
    }

    size_t currentCount = 0;
    while (currentCount < count)
    {
        Data *data = NULL;
        if (xQueueReceive(queue, &data, 0) == pdTRUE)
        {
            fwrite(data, sizeof(Data), 1, f);
            free(data);
            currentCount++;
        }
        else
        {
            break;
        }
    }

    ESP_LOGI(TAG, "Written %d items to %s", currentCount, fname);
    fclose(f);
    free(fname);

#ifdef LED_TWO_LEDS
    ledSetColor(GREEN, 0);
#endif
}

void datastoreRead(QueueHandle_t queue, size_t maxCount)
{
    if (!spiffsMounted)
    {
        ESP_LOGE(TAG, "SPIFFS not mounted");
        return;
    }

#ifdef LED_TWO_LEDS
    ledSetColor(CYAN, 0);
#endif

    ESP_LOGI(TAG, "Reading from SPIFFS");

    DIR *d = opendir("/spiffs");
    if (d == NULL)
    {
        ESP_LOGE(TAG, "Failed to open directory");

#ifdef LED_TWO_LEDS
        ledSetColor(RED, 0);
#endif

        return;
    }

    struct dirent *de;
    while ((de = readdir(d)) != NULL)
    {
        if (de->d_type == DT_REG)
        {
            char *fname = malloc(32);
            sprintf(fname, "/spiffs/%s", de->d_name);

            struct stat st;
            if (stat(fname, &st) != 0)
            {
                ESP_LOGE(TAG, "Failed to stat file %s", fname);
                free(fname);
                continue;
            }

            if (st.st_size % sizeof(Data) != 0 || st.st_size == 0)
            {
                ESP_LOGE(TAG, "File %s has invalid size %d, deleting", fname, st.st_size);
                unlink(fname);
                free(fname);
                continue;
            }

            if (st.st_size / sizeof(Data) > maxCount)
            {
                ESP_LOGW(TAG, "File %s has %d items, but maxCount is %d, skipping", fname, st.st_size / sizeof(Data), maxCount);
                free(fname);
                continue;
            }

            FILE *f = fopen(fname, "rb");
            if (f == NULL)
            {
                ESP_LOGE(TAG, "Failed to open file %s for reading", fname);
                free(fname);
                continue;
            }

            ESP_LOGI(TAG, "Reading %d items from %s", st.st_size / sizeof(Data), fname);

            size_t currentCount = 0;
            while (currentCount < st.st_size / sizeof(Data))
            {
                Data *data = malloc(sizeof(Data));
                if (fread(data, sizeof(Data), 1, f) != 1)
                {
                    ESP_LOGE(TAG, "Failed to read item %d from %s", currentCount, fname);
                    free(data);
                    break;
                }

                if (xQueueSend(queue, &data, 0) != pdTRUE)
                {
                    ESP_LOGE(TAG, "Failed to send item %d from %s to queue", currentCount, fname);
                    free(data);
                    break;
                }

                currentCount++;
            }

            fclose(f);
            unlink(fname);
            free(fname);
            break;
        }
    }

    closedir(d);

#ifdef LED_TWO_LEDS
    ledSetColor(GREEN, 0);
#endif
}