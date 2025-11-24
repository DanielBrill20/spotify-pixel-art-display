#include "wifi_manager.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAILED_BIT BIT1

static const char* WIFI_TAG = "wifi station";

// FreeRTOS event group
static EventGroupHandle_t wifi_event_group;

static uint8_t connection_attempts = 0;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_ERROR_CHECK(esp_wifi_connect());
        ESP_LOGI(WIFI_TAG, "Connection to AP started successfully");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (connection_attempts < 5) {
            ESP_LOGW(WIFI_TAG, "Failed to connect to the AP, retrying...");
            ESP_ERROR_CHECK(esp_wifi_connect());
            connection_attempts++;
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAILED_BIT);
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(WIFI_TAG, "Internal connection successful, starting DHCP client, no action needed");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "Successfully retrieved IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        connection_attempts = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void initialize_nvs()
{
    esp_err_t error_code = nvs_flash_init();
    if (error_code == ESP_ERR_NVS_NO_FREE_PAGES || error_code == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        error_code = nvs_flash_init();
    }
    ESP_ERROR_CHECK(error_code);
}

static void initialize_wifi_station()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t* netif = esp_netif_create_default_wifi_sta();
    if (netif == NULL) {
        ESP_LOGE(WIFI_TAG, "Failed to create default Wi-Fi station interface");
    }
}

static void initialize_event_handlers()
{
    // These variables store the event handler instances.
    // They are not used in this project and are only useful for
    // unregistering the handlers in the future. Without them,
    // you can just pass NULL to the instance register functions
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &event_handler,
            NULL,
            &instance_any_id
        )
    );
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &event_handler,
            NULL,
            &instance_got_ip
        )
    );
}

static void configure_wifi()
{
    wifi_init_config_t wifi_stack_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_stack_config));

    wifi_config_t wifi_station_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_station_config));
}

static void start_wifi()
{
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void wait_for_connection()
{
    EventBits_t wifi_event_bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAILED_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    if (wifi_event_bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(WIFI_TAG, "Connected to network %s", CONFIG_WIFI_SSID);
    } else if (wifi_event_bits & WIFI_FAILED_BIT) {
        ESP_LOGE(WIFI_TAG, "Failed to connect to network %s", CONFIG_WIFI_SSID);
    } else {
        ESP_LOGE(WIFI_TAG, "Unexpected event, wifi setup neither connected nor failed.");
    }
}

void wifi_manager_init()
{
    // Create the event group
    wifi_event_group = xEventGroupCreate();
    if (wifi_event_group == NULL) {
        ESP_LOGE(WIFI_TAG, "Failed to create event group, aborting.");
        return;
    }

    initialize_nvs();
    initialize_wifi_station();
    initialize_event_handlers();
    configure_wifi();
    start_wifi();
    wait_for_connection();
}
