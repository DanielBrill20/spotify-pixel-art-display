#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief   Connects the ESP as a WiFi station to an access point specified by menuconfig.
 *          Handles all steps, including initializing NVS, initializing the WiFi station
 *          and event loop, setting up WiFi and IP event handlers, configuring the network
 *          with the provided SSID, password, and minimum security threshold of WIFI_AUTH_WPA2_PSK,
 *          and finally connecting the station to the access point.
 *          It also utilizes FreeRTOS wait bits to halt program execution until connection is finished
 *          and an IP address is obtained. Once finished, it establishes an mDNS hostname specified by menuconfig.
 * 
 * @returns `ESP_OK` if WiFi connection successful, otherwise `ESP_FAIL`.
 */
esp_err_t wifi_manager_init();

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
