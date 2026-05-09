/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Hiroki Kawakami
 */

#include "bsp_airq.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

#define BSP_RETURN_ERR(e) do { if (e != ESP_OK) return e; } while (0)

#define I2C0_PORT_NUM

static const char *TAG = "BSP_AIRQ";
static i2c_master_bus_handle_t i2c0;
scd4x_handle_t bsp_airq_scd4x;
sen5x_handle_t bsp_airq_sen5x;

esp_err_t bsp_airq_init(void) {
    esp_err_t err;

    // SEN55 has a dedicated power enable pin (active LOW)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_10),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(GPIO_NUM_10, 0);
    vTaskDelay(pdMS_TO_TICKS(100));  // wait for SEN55 to power up

    err = i2c_new_master_bus(&(i2c_master_bus_config_t){
        .i2c_port = 0,
        .sda_io_num = GPIO_NUM_11,
        .scl_io_num = GPIO_NUM_12,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .flags.enable_internal_pullup = true,
    }, &i2c0);
    BSP_RETURN_ERR(err);

    bsp_airq_scd4x = scd4x_init(i2c0, SCD40_I2C_ADDR_62);
    if (bsp_airq_scd4x == NULL) return ESP_FAIL;
    scd4x_stop_periodic_measurement(bsp_airq_scd4x);

    bsp_airq_sen5x = sen5x_init(i2c0, SEN5X_I2C_ADDR);
    if (bsp_airq_sen5x == NULL) return ESP_FAIL;
    sen5x_device_reset(bsp_airq_sen5x);

    ESP_LOGI(TAG, "AirQ BSP Initialized");
    return ESP_OK;
}
