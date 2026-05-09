#include "exporter_app.hpp"
#include "lvgl.hpp"
#include "platform_port.hpp"
#include "esp_log.h"

static const char *TAG = "exporter_app";

static lv_obj_t *label_co2 = nullptr;
static lv_obj_t *label_temp = nullptr;
static lv_obj_t *label_hum = nullptr;

void show_sensor_value() {
    if (!label_co2) {
        lv_obj_t *scr = lv_screen_active();
        label_co2 = lv_label_create(scr);
        lv_obj_align(label_co2, LV_ALIGN_TOP_MID, 0, 30);
        lv_label_set_text(label_co2, "CO2: ---");
        label_temp = lv_label_create(scr);
        lv_obj_align(label_temp, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label_temp, "Temp: ---");
        label_hum = lv_label_create(scr);
        lv_obj_align(label_hum, LV_ALIGN_BOTTOM_MID, 0, -30);
        lv_label_set_text(label_hum, "Hum: ---");
    }

    bool data_ready = false;
    if (!scd4x_get_data_ready_status(bsp_airq_scd4x, &data_ready) || !data_ready) {
        return;
    }

    uint16_t co2;
    float temperature, humidity;
    if (!scd4x_read_measurement(bsp_airq_scd4x, &co2, &temperature, &humidity)) {
        ESP_LOGE(TAG, "SCD4x read failed");
        return;
    }

    ESP_LOGI(TAG, "CO2: %u ppm, Temp: %.1f C, Hum: %.1f%%", co2, temperature, humidity);

    lv_label_set_text_fmt(label_co2, "CO2: %u ppm", co2);
    lv_label_set_text_fmt(label_temp, "Temp: %.1f C", temperature);
    lv_label_set_text_fmt(label_hum, "Hum: %.1f%%", humidity);
    epd_refresh();
}

void exporter_app() {
    scd4x_start_periodic_measurement(bsp_airq_scd4x);

    lv_lock();
    lv_async_call([]() {
        show_sensor_value();
        lv_timer_create([](lv_timer_t *) { show_sensor_value(); }, 5000, nullptr);
    });
    lv_unlock();
}
