#include "exporter_app.hpp"
#include "lvgl.hpp"
#include "platform_port.hpp"

static const char *TAG = "exporter_app";

// SCD4x labels
static lv_obj_t *label_co2  = nullptr;
static lv_obj_t *label_temp = nullptr;
static lv_obj_t *label_hum  = nullptr;

// SEN5x labels
static lv_obj_t *label_sen5x_temp = nullptr;
static lv_obj_t *label_sen5x_hum  = nullptr;
static lv_obj_t *label_pm2p5      = nullptr;
static lv_obj_t *label_pm10p0     = nullptr;
static lv_obj_t *label_voc        = nullptr;
static lv_obj_t *label_nox        = nullptr;

static lv_obj_t *make_label(lv_obj_t *scr, lv_align_t align, int32_t x, int32_t y, const char *text) {
    lv_obj_t *label = lv_label_create(scr);
    lv_obj_align(label, align, x, y);
    lv_label_set_text(label, text);
    return label;
}

void show_sensor_value() {
    if (!label_co2) {
        lv_obj_t *scr = lv_screen_active();
        // Layout for 200×200 e-paper, 9 rows with ~21px spacing
        label_co2        = make_label(scr, LV_ALIGN_TOP_MID, 0,   2, "CO2: ---");
        label_temp       = make_label(scr, LV_ALIGN_TOP_MID, 0,  23, "Temp: ---");
        label_hum        = make_label(scr, LV_ALIGN_TOP_MID, 0,  44, "Hum: ---");
        label_sen5x_temp = make_label(scr, LV_ALIGN_TOP_MID, 0,  65, "S5-Temp: ---");
        label_sen5x_hum  = make_label(scr, LV_ALIGN_TOP_MID, 0,  86, "S5-Hum: ---");
        label_pm2p5      = make_label(scr, LV_ALIGN_TOP_MID, 0, 107, "PM2.5: ---");
        label_pm10p0     = make_label(scr, LV_ALIGN_TOP_MID, 0, 128, "PM10: ---");
        label_voc        = make_label(scr, LV_ALIGN_TOP_MID, 0, 149, "VOC: ---");
        label_nox        = make_label(scr, LV_ALIGN_TOP_MID, 0, 170, "NOx: ---");
    }

    // SCD4x
    bool scd4x_ready = false;
    if (scd4x_get_data_ready_status(bsp_airq_scd4x, &scd4x_ready) && scd4x_ready) {
        uint16_t co2;
        float temperature, humidity;
        if (scd4x_read_measurement(bsp_airq_scd4x, &co2, &temperature, &humidity)) {
            LOG_I(TAG, "SCD4x  CO2: %u ppm, Temp: %.1f C, Hum: %.1f%%", co2, temperature, humidity);
            lv_label_set_text_fmt(label_co2,  "CO2: %u ppm", co2);
            lv_label_set_text_fmt(label_temp, "Temp: %.1f C", temperature);
            lv_label_set_text_fmt(label_hum,  "Hum: %.1f%%", humidity);
        } else {
            LOG_E(TAG, "SCD4x read failed");
        }
    }

    // SEN5x
    bool sen5x_ready = false;
    if (!sen5x_read_data_ready(bsp_airq_sen5x, &sen5x_ready)) {
        LOG_E(TAG, "SEN5x data_ready I2C error");
    } else if (!sen5x_ready) {
        LOG_D(TAG, "SEN5x data not ready");
    } else {
        float pm1p0, pm2p5, pm4p0, pm10p0, hum, temp, voc, nox;
        if (sen5x_read_measured_values(bsp_airq_sen5x,
                &pm1p0, &pm2p5, &pm4p0, &pm10p0, &hum, &temp, &voc, &nox)) {
            LOG_I(TAG, "SEN5x  PM2.5: %.1f, PM10: %.1f, Temp: %.1f, Hum: %.1f, VOC: %.1f, NOx: %.1f",
                     pm2p5, pm10p0, temp, hum, voc, nox);
            lv_label_set_text_fmt(label_sen5x_temp, "S5-Temp: %.1f C", temp);
            lv_label_set_text_fmt(label_sen5x_hum,  "S5-Hum: %.1f%%", hum);
            lv_label_set_text_fmt(label_pm2p5,      "PM2.5: %.1f ug/m3", pm2p5);
            lv_label_set_text_fmt(label_pm10p0,     "PM10:  %.1f ug/m3", pm10p0);
            lv_label_set_text_fmt(label_voc,        "VOC: %.1f", voc);
            lv_label_set_text_fmt(label_nox,        "NOx: %.1f", nox);
        } else {
            LOG_E(TAG, "SEN5x read values failed");
        }
    }

    epd_refresh();
}

void exporter_app() {
    scd4x_start_periodic_measurement(bsp_airq_scd4x);

    sen5x_start_measurement(bsp_airq_sen5x);

    lv_lock();
    lv_async_call([]() {
        show_sensor_value();
        lv_timer_create([](lv_timer_t *) { show_sensor_value(); }, 5000, nullptr);
    });
    lv_unlock();
}
