#ifndef MATRIX_DRIVER_H
#define MATRIX_DRIVER_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "esp_err.h"
#include "esp_log.h"

esp_err_t display_image();
esp_err_t display_screensaver();

#endif // MATRIX_DRIVER_H
