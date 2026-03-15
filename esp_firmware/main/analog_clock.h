#ifndef ANALOG_CLOCK_H
#define ANALOG_CLOCK_H

#ifdef __cplusplus
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
extern "C" {
#endif

#include <esp_err.h>

/**
 * @brief   Begins the analog clock screensaver.
 *          Creates a FreeRTOS task that runs every second,
 *          getting the current time and drawing the clock.
 * 
 * @param   matrix The HUB75 LED matrix to draw to.
 * 
 * @returns `ESP_OK` if the task started successfully, otherwise `ESP_FAIL`.
 */
esp_err_t run_analog_clock(MatrixPanel_I2S_DMA* matrix);

/**
 * @brief   Stops the analog clock screensaver by deleting the underlying FreeRTOS task.
 * 
 * @returns `ESP_FAIL` if the task was already deleted
 *          (meaning this function was either called twice or the clock was not previously started),
 *          otherwise `ESP_OK`.
 */
esp_err_t stop_analog_clock();

/**
 * @brief   Initializes 2 arrays used for effecient drawing of the analog clock screensaver.
 *          Specifically, the arrays contain x and y coordinates for the hour-marker dots.
 *          This must be run once before `run_analog_clock` can be called.
 */
void analog_clock_init();

#ifdef __cplusplus
}
#endif

#endif // ANALOG_CLOCK_H
