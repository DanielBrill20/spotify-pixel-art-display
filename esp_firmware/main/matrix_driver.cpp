#include "matrix_driver.h"
#include "life_screensaver.h"
#include "http_server.h"
#include <cstdint>
#include "esp_log.h"

static const char* MATRIX_TAG = "matrix driver";

MatrixPanel_I2S_DMA* matrix = nullptr;

static esp_err_t stop_screensaver()
{
    stop_game_of_life();
    matrix->clearScreen();
    matrix->flipDMABuffer();
    return ESP_OK;
}

static esp_err_t matrix_driver_deinit()
{
    if (matrix) {
        delete matrix;
        matrix = nullptr;
        ESP_LOGI(MATRIX_TAG, "Matrix deinitialized");
    }
    return ESP_OK;
}

esp_err_t display_image()
{
    if (!matrix) {
        ESP_LOGE(MATRIX_TAG, "Matrix uninitialized");
        return ESP_FAIL;
    }
    ESP_ERROR_CHECK(stop_screensaver());
    for (uint16_t i = 0; i < IMAGE_SIZE; i += 3) {
        const uint16_t pixel = i / 3;
        const uint16_t x = pixel % CONFIG_PANEL_WIDTH;
        const uint16_t y = pixel / CONFIG_PANEL_WIDTH;
        matrix->drawPixelRGB888(x, y,
                                image_buf[i],
                                image_buf[i+1],
                                image_buf[i+2]);
    }
    matrix->flipDMABuffer();
    ESP_LOGI(MATRIX_TAG, "Image drawn successfully");
    return ESP_OK;
}

esp_err_t display_screensaver()
{
    if (!matrix) {
        ESP_LOGE(MATRIX_TAG, "Matrix uninitialized");
        return ESP_FAIL;
    }
    ESP_ERROR_CHECK(run_game_of_life());
    return ESP_OK;
}

esp_err_t matrix_driver_init()
{
    ESP_ERROR_CHECK(matrix_driver_deinit());
    HUB75_I2S_CFG::i2s_pins _pins = {CONFIG_R1, CONFIG_G1,
                                    CONFIG_B1, CONFIG_R2,
                                    CONFIG_G2, CONFIG_B2,
                                    CONFIG_A, CONFIG_B,
                                    CONFIG_C, CONFIG_D,
                                    CONFIG_E, CONFIG_LAT,
                                    CONFIG_OE, CONFIG_CLK};
    HUB75_I2S_CFG config(CONFIG_PANEL_WIDTH, CONFIG_PANEL_HEIGHT,
                        CONFIG_PANEL_CHAIN, _pins);
    config.clkphase = false;
    config.double_buff = true;
    matrix = new MatrixPanel_I2S_DMA(config);
    if (!matrix) {
        ESP_LOGE(MATRIX_TAG, "Matrix allocation failed");
        return ESP_ERR_NO_MEM;
    }
    if (!matrix->begin()) {
        ESP_LOGE(MATRIX_TAG, "Matrix failed to begin");
        ESP_ERROR_CHECK(matrix_driver_deinit());
        return ESP_FAIL;
    }
    matrix->clearScreen();
    matrix->setPanelBrightness(CONFIG_PANEL_BRIGHTNESS);
    ESP_LOGI(MATRIX_TAG, "Matrix initialized: %dx%d, %d chained panels",
        CONFIG_PANEL_WIDTH, CONFIG_PANEL_HEIGHT, CONFIG_PANEL_CHAIN);
    return ESP_OK;
}
