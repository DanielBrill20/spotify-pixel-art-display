#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "mdns.h"

/**
 * @brief   Connects the ESP as a WiFi station to an access point specified by menuconfig.
 *          Handles all steps, including initializing NVS, initializing the WiFi station
 *          and event loop, setting up WiFi and IP event handlers, configuring the network
 *          with the provided SSID, password, and minimum security threshold of WIFI_AUTH_WPA2_PSK,
 *          and finally connecting the station to the access point.
 *          It also utilizes FreeRTOS wait bits to halt program execution until connection is finished
 *          and an IP address is obtained. Once finished, it establishes an mDNS hostname specified by menuconfig.
 */
void wifi_manager_init();

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
