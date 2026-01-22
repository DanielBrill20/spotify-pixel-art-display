#ifndef MATRIX_DRIVER_H
#define MATRIX_DRIVER_H

#ifdef __cplusplus
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
/**
 * @brief   Global pointer to a single HUB75 LED matrix, initialized by `matrix_driver_init()`.
 */
extern MatrixPanel_I2S_DMA* matrix;
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief   Displays an image on the matrix as stored in `image_buf` from `http_server.h`.
 * 
 * @returns `ESP_OK` if image display is successful, otherwise `ESP_FAIL` if matrix is uninitialized.
 */
esp_err_t display_image();

/**
 * @brief   Displays the screensaver. (Currently means running the Conway's Game of Life Screensaver)
 * 
 * @returns `ESP_OK` if screensaver mode succeeds, otherwise `ESP_FAIL` if matrix is uninitialized.
 */
esp_err_t display_screensaver();

/**
 * @brief   Initializes the HUB75 LED Matrix with a custom configuration for
 *          width, height, chained panels, and pinout as defined in `idf.py menuconfig`.
 *          Sets clock phase to false for panels with a "negative clock edge".
 *          Enables double buffer for smoother image transitions.
 *          Starts matrix with a clear screen and proper brightness.
 * 
 * @returns `ESP_OK` if panel initialization is successful,
 *          `ESP_ERR_NO_MEM` if `MatrixPanel_I2S_DMA` allocation fails,
 *          otherwise `ESP_FAIL`.
 */
esp_err_t matrix_driver_init();

#ifdef __cplusplus
}
#endif

#endif // MATRIX_DRIVER_H
