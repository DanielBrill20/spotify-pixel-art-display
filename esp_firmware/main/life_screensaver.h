#ifndef LIFE_SCREENSAVER_H
#define LIFE_SCREENSAVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief   Begins the Conway's Game of Life screensaver.
 * 
 * @returns `ESP_OK` if game starts successfully, otherwise `ESP_FAIL` if matrix is uninitialized.
 */
esp_err_t run_game_of_life();

/**
 * @brief   Stops the Conway's Game of Life screensaver, stopping the timer and clearing the matrix.
 * 
 * @returns `ESP_OK` if game stops successfully, otherwise `ESP_FAIL` if matrix is uninitialized.
 */
esp_err_t stop_game_of_life();

#ifdef __cplusplus
}
#endif

#endif // LIFE_SCREENSAVER_H
