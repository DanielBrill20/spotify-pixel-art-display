#include "matrix_driver.h"
#include "http_server.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "esp_log.h"

static const char* MATRIX_TAG = "matrix driver";

static MatrixPanel_I2S_DMA* matrix = nullptr;

esp_err_t display_image()
{
    for (uint16_t i = 0; i < IMAGE_SIZE; i++) {
        const uint8_t r = image_buf[i];
        const uint8_t g = image_buf[i+1];
        const uint8_t b = image_buf[i+2];
        const uint8_t x = (i / 3) % CONFIG_PANEL_WIDTH;
        const uint8_t y = (i / 3) / CONFIG_PANEL_WIDTH;
    }
    return ESP_OK;
}

esp_err_t display_screensaver()
{
    //
    return ESP_OK;
}

esp_err_t matrix_driver_init()
{
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
    matrix = new MatrixPanel_I2S_DMA(config);
    if (!matrix) {
        ESP_LOGE(MATRIX_TAG, "Matrix allocation failed");
        return ESP_ERR_NO_MEM;
    }
    if (!matrix->begin()) {
        ESP_LOGE(MATRIX_TAG, "Matrix failed to begin");
        delete matrix;
        matrix = nullptr;
        return ESP_FAIL;
    }
    matrix->clearScreen();
    matrix->setPanelBrightness(40);
    ESP_LOGI(MATRIX_TAG, "Matrix initialized: %dx%d, %d chained panels",
        CONFIG_PANEL_WIDTH, CONFIG_PANEL_HEIGHT, CONFIG_PANEL_CHAIN);
    return ESP_OK;
}
