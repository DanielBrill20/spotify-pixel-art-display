#include "wifi_manager.h"
#include "http_server.h"

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(wifi_manager_init());
    ESP_ERROR_CHECK(http_server_init());
}
