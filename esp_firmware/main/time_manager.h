#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief   Initializes and synchronizes the system clock.
 *          Utilizes the Simple Network Time Protocol (SNTP) to get UTC time.
 *          Configures the timezone to US Eastern Time (EST/EDT).
 * 
 * @returns `ESP_OK` if configuration is successful,
 *          `ESP_FAIL` if the SNTP sync is not successful within 10 seconds.
 */
esp_err_t time_manager_init();

#ifdef __cplusplus
}
#endif

#endif // TIME_MANAGER_H
