/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Hiroki Kawakami
 */

#include "scd4x.h"
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_TIMEOUT_MS 100

struct scd4x {
    i2c_master_dev_handle_t i2c_dev;
};

// CRC-8, polynomial 0x31, init 0xFF
static uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static bool write_cmd(scd4x_handle_t handle, uint16_t cmd) {
    uint8_t buf[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };
    return i2c_master_transmit(handle->i2c_dev, buf, 2, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) == ESP_OK;
}

static bool write_cmd_u16(scd4x_handle_t handle, uint16_t cmd, uint16_t value) {
    uint8_t buf[5];
    buf[0] = cmd >> 8;
    buf[1] = cmd & 0xFF;
    buf[2] = value >> 8;
    buf[3] = value & 0xFF;
    buf[4] = crc8(&buf[2], 2);
    return i2c_master_transmit(handle->i2c_dev, buf, 5, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) == ESP_OK;
}

static bool read_words(scd4x_handle_t handle, uint16_t *words, size_t count) {
    uint8_t rx[9]; // max 3 words × 3 bytes
    if (i2c_master_receive(handle->i2c_dev, rx, count * 3, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) != ESP_OK) {
        return false;
    }
    for (size_t i = 0; i < count; i++) {
        uint8_t *p = &rx[i * 3];
        if (crc8(p, 2) != p[2]) return false;
        words[i] = ((uint16_t)p[0] << 8) | p[1];
    }
    return true;
}

// Send command, wait, then read words
static bool cmd_read(scd4x_handle_t handle, uint16_t cmd, uint32_t delay_ms, uint16_t *words, size_t count) {
    if (!write_cmd(handle, cmd)) return false;
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    return read_words(handle, words, count);
}

// Send command with data, wait, then read words
static bool cmd_write_read(scd4x_handle_t handle, uint16_t cmd, uint16_t value, uint32_t delay_ms, uint16_t *words, size_t count) {
    if (!write_cmd_u16(handle, cmd, value)) return false;
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    return read_words(handle, words, count);
}

scd4x_handle_t scd4x_init(i2c_master_bus_handle_t i2c_bus, uint8_t i2c_address) {
    struct scd4x *handle = (struct scd4x*)malloc(sizeof(struct scd4x));
    if (!handle) return NULL;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2c_address,
        .scl_speed_hz = 100000,
    };
    if (i2c_master_bus_add_device(i2c_bus, &dev_cfg, &handle->i2c_dev) != ESP_OK) {
        free(handle);
        return NULL;
    }
    return handle;
}

void scd4x_deinit(scd4x_handle_t handle) {
    if (!handle) return;
    i2c_master_bus_rm_device(handle->i2c_dev);
    free(handle);
}

bool scd4x_start_periodic_measurement(scd4x_handle_t handle) {
    return write_cmd(handle, 0x21b1);
}

bool scd4x_stop_periodic_measurement(scd4x_handle_t handle) {
    if (!write_cmd(handle, 0x3f86)) return false;
    vTaskDelay(pdMS_TO_TICKS(500));
    return true;
}

bool scd4x_start_low_power_periodic_measurement(scd4x_handle_t handle) {
    return write_cmd(handle, 0x21ac);
}

bool scd4x_read_measurement(scd4x_handle_t handle, uint16_t *co2_concentration, float *temperature, float *relative_humidity) {
    uint16_t words[3];
    if (!cmd_read(handle, 0xec05, 1, words, 3)) return false;
    if (co2_concentration) *co2_concentration = words[0];
    if (temperature) *temperature = -45.0f + (175.0f * words[1]) / 65535.0f;
    if (relative_humidity) *relative_humidity = (100.0f * words[2]) / 65535.0f;
    return true;
}

bool scd4x_get_data_ready_status(scd4x_handle_t handle, bool *data_ready_status) {
    uint16_t word;
    if (!cmd_read(handle, 0xe4b8, 1, &word, 1)) return false;
    *data_ready_status = (word & 0x07FF) != 0;
    return true;
}

bool scd4x_set_temperature_offset(scd4x_handle_t handle, float temperature_offset) {
    uint16_t raw = (uint16_t)((temperature_offset * 65535.0f) / 175.0f + 0.5f);
    if (!write_cmd_u16(handle, 0x241d, raw)) return false;
    vTaskDelay(pdMS_TO_TICKS(1));
    return true;
}

bool scd4x_get_temperature_offset(scd4x_handle_t handle, float *temperature_offset) {
    uint16_t word;
    if (!cmd_read(handle, 0x2318, 1, &word, 1)) return false;
    *temperature_offset = (175.0f * word) / 65535.0f;
    return true;
}

bool scd4x_set_sensor_altitude(scd4x_handle_t handle, uint16_t altitude_m) {
    if (!write_cmd_u16(handle, 0x2427, altitude_m)) return false;
    vTaskDelay(pdMS_TO_TICKS(1));
    return true;
}

bool scd4x_get_sensor_altitude(scd4x_handle_t handle, uint16_t *altitude_m) {
    return cmd_read(handle, 0x2322, 1, altitude_m, 1);
}

bool scd4x_set_ambient_pressure(scd4x_handle_t handle, float ambient_pressure_pa) {
    uint16_t raw = (uint16_t)(ambient_pressure_pa / 100.0f + 0.5f);
    if (!write_cmd_u16(handle, 0xe000, raw)) return false;
    vTaskDelay(pdMS_TO_TICKS(1));
    return true;
}

bool scd4x_get_ambient_pressure(scd4x_handle_t handle, float *ambient_pressure_pa) {
    uint16_t word;
    if (!cmd_read(handle, 0xe000, 1, &word, 1)) return false;
    *ambient_pressure_pa = (float)word * 100.0f;
    return true;
}

bool scd4x_set_automatic_self_calibration_enabled(scd4x_handle_t handle, bool enabled) {
    if (!write_cmd_u16(handle, 0x2416, enabled ? 1 : 0)) return false;
    vTaskDelay(pdMS_TO_TICKS(1));
    return true;
}

bool scd4x_get_automatic_self_calibration_enabled(scd4x_handle_t handle, bool *enabled) {
    uint16_t word;
    if (!cmd_read(handle, 0x2313, 1, &word, 1)) return false;
    *enabled = word != 0;
    return true;
}

bool scd4x_set_automatic_self_calibration_target(scd4x_handle_t handle, uint16_t target_co2_ppm) {
    if (!write_cmd_u16(handle, 0x243a, target_co2_ppm)) return false;
    vTaskDelay(pdMS_TO_TICKS(1));
    return true;
}

bool scd4x_get_automatic_self_calibration_target(scd4x_handle_t handle, uint16_t *target_co2_ppm) {
    return cmd_read(handle, 0x233f, 1, target_co2_ppm, 1);
}

bool scd4x_set_automatic_self_calibration_initial_period(scd4x_handle_t handle, uint16_t hours) {
    if (!write_cmd_u16(handle, 0x2445, hours)) return false;
    vTaskDelay(pdMS_TO_TICKS(1));
    return true;
}

bool scd4x_get_automatic_self_calibration_initial_period(scd4x_handle_t handle, uint16_t *hours) {
    return cmd_read(handle, 0x2340, 1, hours, 1);
}

bool scd4x_set_automatic_self_calibration_standard_period(scd4x_handle_t handle, uint16_t hours) {
    if (!write_cmd_u16(handle, 0x244e, hours)) return false;
    vTaskDelay(pdMS_TO_TICKS(1));
    return true;
}

bool scd4x_get_automatic_self_calibration_standard_period(scd4x_handle_t handle, uint16_t *hours) {
    return cmd_read(handle, 0x234b, 1, hours, 1);
}

bool scd4x_perform_forced_recalibration(scd4x_handle_t handle, uint16_t target_co2_ppm, uint16_t *frc_correction) {
    return cmd_write_read(handle, 0x362f, target_co2_ppm, 400, frc_correction, 1);
}

bool scd4x_persist_settings(scd4x_handle_t handle) {
    if (!write_cmd(handle, 0x3615)) return false;
    vTaskDelay(pdMS_TO_TICKS(800));
    return true;
}

bool scd4x_get_serial_number(scd4x_handle_t handle, uint64_t *serial_number) {
    uint16_t words[3];
    if (!cmd_read(handle, 0x3682, 1, words, 3)) return false;
    *serial_number = ((uint64_t)words[0] << 32) | ((uint64_t)words[1] << 16) | words[2];
    return true;
}

bool scd4x_perform_self_test(scd4x_handle_t handle, uint16_t *sensor_status) {
    return cmd_read(handle, 0x3639, 10000, sensor_status, 1);
}

bool scd4x_perform_factory_reset(scd4x_handle_t handle) {
    if (!write_cmd(handle, 0x3632)) return false;
    vTaskDelay(pdMS_TO_TICKS(1200));
    return true;
}

bool scd4x_reinit(scd4x_handle_t handle) {
    if (!write_cmd(handle, 0x3646)) return false;
    vTaskDelay(pdMS_TO_TICKS(30));
    return true;
}

bool scd4x_get_sensor_variant(scd4x_handle_t handle, scd4x_sensor_variant_t *sensor_variant) {
    uint16_t word;
    if (!cmd_read(handle, 0x202f, 1, &word, 1)) return false;
    uint16_t masked = word & (uint16_t)SCD4X_SENSOR_VARIANT_MASK;
    switch (masked) {
        case (uint16_t)SCD4X_SENSOR_VARIANT_SCD40: *sensor_variant = SCD4X_SENSOR_VARIANT_SCD40; break;
        case (uint16_t)SCD4X_SENSOR_VARIANT_SCD41: *sensor_variant = SCD4X_SENSOR_VARIANT_SCD41; break;
        case (uint16_t)SCD4X_SENSOR_VARIANT_SCD42: *sensor_variant = SCD4X_SENSOR_VARIANT_SCD42; break;
        case (uint16_t)SCD4X_SENSOR_VARIANT_SCD43: *sensor_variant = SCD4X_SENSOR_VARIANT_SCD43; break;
        default: *sensor_variant = SCD4X_SENSOR_VARIANT_MASK; break;
    }
    return true;
}

bool scd4x_measure_single_shot(scd4x_handle_t handle) {
    if (!write_cmd(handle, 0x219d)) return false;
    vTaskDelay(pdMS_TO_TICKS(5000));
    return true;
}

bool scd4x_measure_single_shot_rht_only(scd4x_handle_t handle) {
    if (!write_cmd(handle, 0x2196)) return false;
    vTaskDelay(pdMS_TO_TICKS(50));
    return true;
}

bool scd4x_power_down(scd4x_handle_t handle) {
    if (!write_cmd(handle, 0x36e0)) return false;
    vTaskDelay(pdMS_TO_TICKS(1));
    return true;
}

bool scd4x_wake_up(scd4x_handle_t handle) {
    // Sensor does not ACK wake_up, so ignore the return value
    write_cmd(handle, 0x36f6);
    vTaskDelay(pdMS_TO_TICKS(30));
    return true;
}
