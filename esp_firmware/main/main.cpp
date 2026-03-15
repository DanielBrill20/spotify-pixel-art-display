#include "wifi_manager.h"
#include "http_server.h"
#include "matrix_driver.h"
#include "time_manager.h"
#include "analog_clock.h"

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(matrix_driver_init());
    ESP_ERROR_CHECK(wifi_manager_init());
    ESP_ERROR_CHECK(http_server_init());
    ESP_ERROR_CHECK(time_manager_init());
    analog_clock_init();
}
