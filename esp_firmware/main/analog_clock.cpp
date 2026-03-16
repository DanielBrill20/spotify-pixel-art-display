#include "analog_clock.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <sdkconfig.h>
#include <time.h>

#define CLOCK_RADIUS (CONFIG_PANEL_WIDTH < CONFIG_PANEL_HEIGHT ? CONFIG_PANEL_WIDTH/2 : CONFIG_PANEL_HEIGHT/2)
#define HR_HAND_LEN (CLOCK_RADIUS*.5)
#define MIN_HAND_LEN (CLOCK_RADIUS*.8)
#define SEC_HAND_LEN (CLOCK_RADIUS*.9)
#define COLOR 0x8410
#ifndef M_PI
#define M_PI 3.141593
#endif

static const char* CLOCK_TAG = "analog clock";

static TaskHandle_t analog_clock_task_handle = NULL;

static int16_t hr_x[12];
static int16_t hr_y[12];

static void calc_hr_coords()
{
    float theta = 0;
    for (uint8_t i = 0; i < 12; i++) {
        int16_t x0 = i < 6 ? CLOCK_RADIUS : CLOCK_RADIUS-1;
        int16_t y0 = (3 <= i && i < 9) ? CLOCK_RADIUS : CLOCK_RADIUS-1;
        hr_x[i] = x0 + (int16_t)round(MIN_HAND_LEN * sin(theta));
        hr_y[i] = y0 - (int16_t)round(MIN_HAND_LEN * cos(theta));
        theta += M_PI/6;
    }
}

static void draw_clock_hand(MatrixPanel_I2S_DMA* matrix, float theta, float len)
{
    int16_t x0 = theta < M_PI ? CLOCK_RADIUS : CLOCK_RADIUS-1;
    int16_t y0 = (M_PI/2 <= theta && theta < 3*M_PI/2) ? CLOCK_RADIUS : CLOCK_RADIUS-1;
    int16_t x1 = x0 + (int16_t)round(len * sin(theta));
    int16_t y1 = y0 - (int16_t)round(len * cos(theta));
    matrix->drawLine(x0, y0, x1, y1, COLOR);
}

static void draw_clock(MatrixPanel_I2S_DMA* matrix, struct tm time)
{
    matrix->clearScreen();

    matrix->drawRoundRect(0, 0, CLOCK_RADIUS*2, CLOCK_RADIUS*2, CLOCK_RADIUS, COLOR);
    for (uint8_t i = 0; i < 12; i++) {
        matrix->drawPixel(hr_x[i], hr_y[i], COLOR);
    }

    float sec_theta = M_PI/30 * time.tm_sec;
    float min_theta = M_PI/30 * time.tm_min;
    float hr_theta = (M_PI/6 * (time.tm_hour % 12)) + (M_PI/360 * time.tm_min);
    draw_clock_hand(matrix, sec_theta, SEC_HAND_LEN);
    draw_clock_hand(matrix, min_theta, MIN_HAND_LEN);
    draw_clock_hand(matrix, hr_theta, HR_HAND_LEN);

    matrix->flipDMABuffer();
}

static void analog_clock_task(void* arg)
{
    MatrixPanel_I2S_DMA* matrix = (MatrixPanel_I2S_DMA*)arg;
    time_t now;
    struct tm time_info;
    TickType_t previous_wake_time = xTaskGetTickCount();

    for ( ;; )
    {
        time(&now);
        localtime_r(&now, &time_info);
        draw_clock(matrix, time_info);

        xTaskDelayUntil(&previous_wake_time, pdMS_TO_TICKS(1000));
    }
}

esp_err_t run_analog_clock(MatrixPanel_I2S_DMA* matrix)
{
    if (analog_clock_task_handle != NULL) {
        ESP_LOGW(CLOCK_TAG, "Analog clock task is already running.");
        return ESP_FAIL;
    }
    BaseType_t returned;
    returned = xTaskCreate(analog_clock_task,
                        "analog clock task",
                        2496, // 20% wiggle room based on calculated 2080 bytes of used stack
                        (void*) matrix,
                        tskIDLE_PRIORITY + 1,
                        &analog_clock_task_handle);
    if (returned != pdPASS) {
        ESP_LOGE(CLOCK_TAG, "Failed to create clock task with error code %d.", returned);
        analog_clock_task_handle = NULL;
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t stop_analog_clock()
{
    if (analog_clock_task_handle == NULL) {
        ESP_LOGW(CLOCK_TAG, "Analog clock task was already deleted!");
        return ESP_FAIL;
    }
    vTaskDelete(analog_clock_task_handle);
    analog_clock_task_handle = NULL;
    return ESP_OK;
}

void analog_clock_init()
{
    calc_hr_coords();
}
