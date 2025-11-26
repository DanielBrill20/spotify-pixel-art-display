#ifndef MATRIX_MANAGER_H
#define MATRIX_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ESP32-HUB75-MatrixPanel-DMA>
#include "esp_err.h"
#include "esp_log.h"

esp_err_t display_image();
esp_err_t display_screensaver();

#ifdef __cplusplus
}
#endif

#endif // MATRIX_MANAGER_H
