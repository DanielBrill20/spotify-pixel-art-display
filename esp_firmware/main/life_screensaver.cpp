#include "life_screensaver.h"
#include "matrix_driver.h"
#include <cstdint>
#include <array>
#include "sdkconfig.h"
#include "esp_random.h"
#include "esp_timer.h"

#define MICROSEC_PER_MS 1000
#define TICK_MS 300
#define COLOR_INCREMENT 15
#define STARTING_DENSITY 0.5
#define MINIMUM_DENSITY 0.05
#define MAXIMUM_DENSITY 1

typedef std::array<std::array<bool, CONFIG_PANEL_WIDTH>, CONFIG_PANEL_HEIGHT> Tick_t;

typedef struct LifeState_t {
    Tick_t current_tick;
    Tick_t next_tick;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

static const char* LIFE_TAG = "life screensaver";

static LifeState_t life_state = {};
static esp_timer_handle_t tick_timer;

static void raindbow_transition(LifeState_t* state)
{
    if (state->r == UINT8_MAX) {
        if (state->b == 0 && state->g != UINT8_MAX) {
            state->g += COLOR_INCREMENT;
            return;
        } else if (state->g == 0) {
            state->b -= COLOR_INCREMENT;
        }
    }
    if (state->g == UINT8_MAX) {
        if (state->r == 0 && state->b != UINT8_MAX) {
            state->b += COLOR_INCREMENT;
            return;
        } else if (state->b == 0) {
            state->r -= COLOR_INCREMENT;
        }
    }
    if (state->b == UINT8_MAX) {
        if (state->g == 0 && state->r != UINT8_MAX) {
            state->r += COLOR_INCREMENT;
            return;
        } else if (state->r == 0) {
            state->g -= COLOR_INCREMENT;
        }
    }
}

static void draw_next_tick(void* arg)
{
    matrix->clearScreen();
    LifeState_t* state = (LifeState_t*)arg;
    raindbow_transition(state);
    for (uint16_t y = 0; y < CONFIG_PANEL_HEIGHT; y++) {
        for (uint16_t x = 0; x < CONFIG_PANEL_WIDTH; x++) {
            uint8_t neighbors = 0;
            bool self = state->current_tick[y][x];
            for (int8_t offy = -1; offy < 2; offy++) {
                for (int8_t offx = -1; offx < 2; offx++) {
                    if (offx == 0 && offy == 0) {
                        continue;
                    }
                    int16_t nx = (x + offx + CONFIG_PANEL_WIDTH) % CONFIG_PANEL_WIDTH;
                    int16_t ny = (y + offy + CONFIG_PANEL_HEIGHT) % CONFIG_PANEL_HEIGHT;
                    if (state->current_tick[ny][nx]) {
                        neighbors++;
                    }
                }
            }
            if (neighbors == 3 || (self && neighbors == 2)) {
                state->next_tick[y][x] = true;
                matrix->drawPixelRGB888(x, y, state->r, state->g, state->b);
            }
        }
    }
    matrix->flipDMABuffer();
    ESP_LOGI(LIFE_TAG,
        "Drew next GoL tick on matrix with color R: %u G: %u B: %u",
        state->r, state->g, state->b);
    state->current_tick = state->next_tick; // Direct assignment, an advantage of std::array
    state->next_tick = {};
}

static void draw_first_tick(float density, LifeState_t* state)
{
    matrix->clearScreen();
    if (density > MAXIMUM_DENSITY) {
        density = MAXIMUM_DENSITY;
    } else if (density < MINIMUM_DENSITY) {
        density = MINIMUM_DENSITY;
    }
    uint32_t cutoff = UINT32_MAX * density;
    for (uint16_t y = 0; y < CONFIG_PANEL_HEIGHT; y++) {
        for (uint16_t x = 0; x < CONFIG_PANEL_WIDTH; x++) {
            if (esp_random() <= cutoff) {
                state->current_tick[y][x] = true;
                matrix->drawPixelRGB888(x, y, state->r, state->g, state->b);
            }
        }
    }
    matrix->flipDMABuffer();
    ESP_LOGI(LIFE_TAG,
        "Drew randomly generated GoL tick on matrix with color R: %u G: %u B: %u",
        state->r, state->g, state->b);
}

esp_err_t run_game_of_life()
{
    // This shouldn't technically be necessary as long as this is only called from
    // `display_screensaver` in `matrix_driver.cpp`, which contains the same check
    if (!matrix) {
        ESP_LOGE(LIFE_TAG, "Attempting to show screensaver on uninitialized matrix");
        return ESP_FAIL;
    }
    life_state.r = UINT8_MAX;
    draw_first_tick(STARTING_DENSITY, &life_state);

    if (!tick_timer) {
        const esp_timer_create_args_t tick_timer_args = {
            .callback = draw_next_tick,
            .arg = &life_state,
            .name = "tick timer"
        };
        ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer));
    }
    esp_timer_start_periodic(tick_timer, TICK_MS * MICROSEC_PER_MS);
    ESP_LOGI(LIFE_TAG, "Started Game of Life screensaver with %d ms tick duration", TICK_MS);
    return ESP_OK;
}

esp_err_t stop_game_of_life()
{
    if (!matrix) {
        ESP_LOGE(LIFE_TAG, "Attempting to show screensaver on uninitialized matrix");
        return ESP_FAIL;
    }
    matrix->clearScreen();
    matrix->flipDMABuffer();
    ESP_ERROR_CHECK(esp_timer_stop(tick_timer));
    life_state = {};
    ESP_LOGI(LIFE_TAG, "Stopped Game of Life screensaver");
    return ESP_OK;
}
