#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "string.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_server.h"

#define IMAGE_SIZE (CONFIG_PANEL_WIDTH * CONFIG_PANEL_HEIGHT * 3)

extern uint8_t image_buf[IMAGE_SIZE];

/**
 * @brief   Starts a simple HTTP server, defines URIs, and registers handlers to handle them.
 */
void http_server_init();

#ifdef __cplusplus
}
#endif

#endif // HTTP_SERVER_H
