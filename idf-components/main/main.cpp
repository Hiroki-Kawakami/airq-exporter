#include "epaper.h"
#include "epaper_lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "exporter_app.hpp"
#include "config.hpp"
#include "platform_port.hpp"

static const char *TAG = "main";
static lv_display_t *disp;

static void lvgl_init() {
    lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 7168,
        .task_affinity = 0,
        .task_max_sleep_ms = 500,
        .task_stack_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_DEFAULT,
        .timer_period_ms = 5,
    };
    auto err = lvgl_port_init(&lvgl_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init LVGL: %s", esp_err_to_name(err));
        return;
    }
}

static void epd_init() {
    epd_config_t epd_cfg = {
        .pins = {
            .busy = EPD_BUSY,
            .rst = EPD_RST,
            .dc = EPD_DC,
            .cs = EPD_CS,
            .sck = EPD_SCLK,
            .mosi = EPD_MOSI,
        },
        .spi = {
            .host = SPI2_HOST,
            .speed_hz = EPD_FREQ,
        },
        .panel = {
            .type = EPD_PANEL_GDEY0154D67,
            .width = 200,
            .height = 200,
            .mirror_x = false,
            .mirror_y = false,
            .rotation = 0,
        },
    };
    epd_handle_t epd = NULL;

    auto ret = epd_init(&epd_cfg, &epd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init e-paper: %s", esp_err_to_name(ret));
        return;
    }

    // Get panel info
    epd_panel_info_t panel_info;
    epd_get_info(epd, &panel_info);
    ESP_LOGI(TAG, "Panel: %dx%d, buffer: %d bytes",
             panel_info.width, panel_info.height, (int)panel_info.buffer_size);

    // Initialize LVGL display with partial refresh and dithering
    epd_lvgl_config_t lvgl_cfg = EPD_LVGL_CONFIG_DEFAULT();
    lvgl_cfg.epd = epd;
    lvgl_cfg.update_mode = EPD_UPDATE_PARTIAL;
    lvgl_cfg.use_partial_refresh = true;
    lvgl_cfg.partial_threshold = 2000;  // Force full refresh every N partial updates
    lvgl_cfg.dither_mode = EPD_DITHER_FLOYD_STEINBERG;  // Enable grayscale dithering

    disp = epd_lvgl_init(&lvgl_cfg);
    if (!disp) {
        ESP_LOGE(TAG, "Failed to init LVGL display");
        epd_deinit(epd);
        return;
    }
}

void epd_refresh(bool full_refresh) {
    if (full_refresh) epd_lvgl_force_full_refresh(disp);
    epd_lvgl_refresh(disp);
}

extern "C" void app_main(void) {
    lvgl_init();
    epd_init();

    auto err = bsp_airq_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AirQ BSP Initialize failed: %s", esp_err_to_name(err));
    }

    exporter_app();
}
