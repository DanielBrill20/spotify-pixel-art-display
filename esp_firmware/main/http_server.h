#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "string.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_server.h"

/**
 * @brief   Starts a simple HTTP server, defines URIs, and registers handlers to handle them.
 */
void http_server_init();

#endif // HTTP_SERVER_H
