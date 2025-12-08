#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "sdkconfig.h"
#include "esp_err.h"

#define IMAGE_SIZE (CONFIG_PANEL_WIDTH * CONFIG_PANEL_HEIGHT * 3)

/**
 * @brief   Global image buffer of size (CONFIG_PANEL_WIDTH * CONFIG_PANEL_HEIGHT * 3).
 *          Used to store album art RGB data in .BSS storage to avoid stack overflow.
 */
extern uint8_t image_buf[IMAGE_SIZE];

/**
 * @brief   Starts a simple HTTP server, defines URIs, and registers handlers to handle them.
 * 
 * @returns `ESP_OK` if server startup successful, otherwise `ESP_FAIL`.
 */
esp_err_t http_server_init();

#ifdef __cplusplus
}
#endif

#endif // HTTP_SERVER_H
