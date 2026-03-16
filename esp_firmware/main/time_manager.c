#include "time_manager.h"
#include <esp_log.h>
#include <esp_netif_sntp.h>
#include <freertos/task.h>
#include <stdlib.h>
#include <time.h>

static const char* TIME_TAG = "time manager";

esp_err_t time_manager_init()
{
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);

    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK)
    {
        ESP_LOGE(TIME_TAG, "Failed to update system time within 10s timeout");
        return ESP_FAIL;
    }

    setenv("TZ", "EST+5EDT,M3.2.0/2,M11.1.0/2", 1);
    tzset();
    return ESP_OK;
}
