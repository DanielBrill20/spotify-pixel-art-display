#ifndef MATRIX_DRIVER_H
#define MATRIX_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

esp_err_t display_image();
esp_err_t display_screensaver();
esp_err_t matrix_driver_init();

#ifdef __cplusplus
}
#endif

#endif // MATRIX_DRIVER_H
