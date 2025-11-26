#include "matrix_driver.h"

#define IMAGE_SIZE (64*64*3) // TO BE DELETED
#define CONFIG_PANEL_WIDTH 64

static MatrixPanel_I2S_DMA *display = nullptr;

esp_err_t display_image() {
    uint8_t image_buf[64*64*3]; // DELETE ONCE CAN USE EXTERN BUFFER
    for (uint16_t i = 0; i < IMAGE_SIZE; i++) {
        const uint8_t r = image_buf[i];
        const uint8_t g = image_buf[i+1];
        const uint8_t b = image_buf[i+2];
        const uint8_t x = (i / 3) % CONFIG_PANEL_WIDTH;
        const uint8_t y = (i / 3) / CONFIG_PANEL_WIDTH;
    }
    return ESP_OK;
}

esp_err_t display_screensaver() {
    //
    return ESP_OK;
}
