/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Hiroki Kawakami
 */

#include "sen5x.h"
#include "sensirion_i2c.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define UINT_INVALID 0xFFFF
#define INT_INVALID  0x7FFF

struct sen5x {
    i2c_master_dev_handle_t i2c_dev;
};

sen5x_handle_t sen5x_init(i2c_master_bus_handle_t i2c_bus, uint8_t i2c_address) {
    struct sen5x *handle = (struct sen5x *)malloc(sizeof(struct sen5x));
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

void sen5x_deinit(sen5x_handle_t handle) {
    if (!handle) return;
    i2c_master_bus_rm_device(handle->i2c_dev);
    free(handle);
}

bool sen5x_start_measurement(sen5x_handle_t handle) {
    if (!sensirion_write_cmd(handle->i2c_dev, 0x0021)) return false;
    vTaskDelay(pdMS_TO_TICKS(50));
    return true;
}

bool sen5x_start_measurement_without_pm(sen5x_handle_t handle) {
    if (!sensirion_write_cmd(handle->i2c_dev, 0x0037)) return false;
    vTaskDelay(pdMS_TO_TICKS(50));
    return true;
}

bool sen5x_stop_measurement(sen5x_handle_t handle) {
    if (!sensirion_write_cmd(handle->i2c_dev, 0x0104)) return false;
    vTaskDelay(pdMS_TO_TICKS(200));
    return true;
}

bool sen5x_read_data_ready(sen5x_handle_t handle, bool *data_ready) {
    uint16_t word;
    if (!sensirion_cmd_read(handle->i2c_dev, 0x0202, 20, &word, 1)) return false;
    *data_ready = (word & 0x00FF) != 0;
    return true;
}

bool sen5x_read_measured_values(sen5x_handle_t handle,
    float *pm1p0, float *pm2p5, float *pm4p0, float *pm10p0,
    float *humidity, float *temperature, float *voc_index, float *nox_index) {
    uint16_t w[8];
    if (!sensirion_cmd_read(handle->i2c_dev, 0x03C4, 20, w, 8)) return false;
    if (pm1p0)       *pm1p0       = w[0] == UINT_INVALID ? NAN : w[0] / 10.0f;
    if (pm2p5)       *pm2p5       = w[1] == UINT_INVALID ? NAN : w[1] / 10.0f;
    if (pm4p0)       *pm4p0       = w[2] == UINT_INVALID ? NAN : w[2] / 10.0f;
    if (pm10p0)      *pm10p0      = w[3] == UINT_INVALID ? NAN : w[3] / 10.0f;
    if (humidity)    *humidity    = (int16_t)w[4] == INT_INVALID ? NAN : (int16_t)w[4] / 100.0f;
    if (temperature) *temperature = (int16_t)w[5] == INT_INVALID ? NAN : (int16_t)w[5] / 200.0f;
    if (voc_index)   *voc_index   = (int16_t)w[6] == INT_INVALID ? NAN : (int16_t)w[6] / 10.0f;
    if (nox_index)   *nox_index   = (int16_t)w[7] == INT_INVALID ? NAN : (int16_t)w[7] / 10.0f;
    return true;
}

bool sen5x_read_measured_pm_values(sen5x_handle_t handle,
    float *pm1p0, float *pm2p5, float *pm4p0, float *pm10p0,
    float *n0p5, float *n1p0, float *n2p5, float *n4p0, float *n10p0,
    float *typical_particle_size_um) {
    uint16_t w[10];
    if (!sensirion_cmd_read(handle->i2c_dev, 0x0413, 20, w, 10)) return false;
    if (pm1p0)                  *pm1p0                  = w[0] == UINT_INVALID ? NAN : w[0] / 10.0f;
    if (pm2p5)                  *pm2p5                  = w[1] == UINT_INVALID ? NAN : w[1] / 10.0f;
    if (pm4p0)                  *pm4p0                  = w[2] == UINT_INVALID ? NAN : w[2] / 10.0f;
    if (pm10p0)                 *pm10p0                 = w[3] == UINT_INVALID ? NAN : w[3] / 10.0f;
    if (n0p5)                   *n0p5                   = w[4] == UINT_INVALID ? NAN : w[4] / 10.0f;
    if (n1p0)                   *n1p0                   = w[5] == UINT_INVALID ? NAN : w[5] / 10.0f;
    if (n2p5)                   *n2p5                   = w[6] == UINT_INVALID ? NAN : w[6] / 10.0f;
    if (n4p0)                   *n4p0                   = w[7] == UINT_INVALID ? NAN : w[7] / 10.0f;
    if (n10p0)                  *n10p0                  = w[8] == UINT_INVALID ? NAN : w[8] / 10.0f;
    if (typical_particle_size_um) *typical_particle_size_um = w[9] == UINT_INVALID ? NAN : w[9] / 1000.0f;
    return true;
}

bool sen5x_set_temperature_offset(sen5x_handle_t handle, float offset_celsius) {
    int16_t raw = (int16_t)(offset_celsius * 200.0f);
    return sen5x_set_temperature_offset_params(handle, raw, 0, 0);
}

bool sen5x_get_temperature_offset(sen5x_handle_t handle, float *offset_celsius) {
    int16_t offset, slope;
    uint16_t time_constant;
    if (!sen5x_get_temperature_offset_params(handle, &offset, &slope, &time_constant)) return false;
    *offset_celsius = (float)offset / 200.0f;
    return true;
}

bool sen5x_set_temperature_offset_params(sen5x_handle_t handle, int16_t offset, int16_t slope, uint16_t time_constant) {
    uint16_t words[3] = { (uint16_t)offset, (uint16_t)slope, time_constant };
    if (!sensirion_write_cmd_words(handle->i2c_dev, 0x60B2, words, 3)) return false;
    vTaskDelay(pdMS_TO_TICKS(20));
    return true;
}

bool sen5x_get_temperature_offset_params(sen5x_handle_t handle, int16_t *offset, int16_t *slope, uint16_t *time_constant) {
    uint16_t words[3];
    if (!sensirion_cmd_read(handle->i2c_dev, 0x60B2, 20, words, 3)) return false;
    *offset        = (int16_t)words[0];
    *slope         = (int16_t)words[1];
    *time_constant = words[2];
    return true;
}

bool sen5x_set_warm_start_parameter(sen5x_handle_t handle, uint16_t warm_start) {
    if (!sensirion_write_cmd_words(handle->i2c_dev, 0x60C6, &warm_start, 1)) return false;
    vTaskDelay(pdMS_TO_TICKS(20));
    return true;
}

bool sen5x_get_warm_start_parameter(sen5x_handle_t handle, uint16_t *warm_start) {
    return sensirion_cmd_read(handle->i2c_dev, 0x60C6, 20, warm_start, 1);
}

bool sen5x_set_voc_algorithm_tuning(sen5x_handle_t handle,
    int16_t index_offset, int16_t learning_time_offset_h, int16_t learning_time_gain_h,
    int16_t gating_max_duration_min, int16_t std_initial, int16_t gain_factor) {
    uint16_t words[6] = {
        (uint16_t)index_offset, (uint16_t)learning_time_offset_h,
        (uint16_t)learning_time_gain_h, (uint16_t)gating_max_duration_min,
        (uint16_t)std_initial, (uint16_t)gain_factor,
    };
    if (!sensirion_write_cmd_words(handle->i2c_dev, 0x60D0, words, 6)) return false;
    vTaskDelay(pdMS_TO_TICKS(20));
    return true;
}

bool sen5x_get_voc_algorithm_tuning(sen5x_handle_t handle,
    int16_t *index_offset, int16_t *learning_time_offset_h, int16_t *learning_time_gain_h,
    int16_t *gating_max_duration_min, int16_t *std_initial, int16_t *gain_factor) {
    uint16_t words[6];
    if (!sensirion_cmd_read(handle->i2c_dev, 0x60D0, 20, words, 6)) return false;
    *index_offset            = (int16_t)words[0];
    *learning_time_offset_h  = (int16_t)words[1];
    *learning_time_gain_h    = (int16_t)words[2];
    *gating_max_duration_min = (int16_t)words[3];
    *std_initial             = (int16_t)words[4];
    *gain_factor             = (int16_t)words[5];
    return true;
}

bool sen5x_set_nox_algorithm_tuning(sen5x_handle_t handle,
    int16_t index_offset, int16_t learning_time_offset_h, int16_t learning_time_gain_h,
    int16_t gating_max_duration_min, int16_t std_initial, int16_t gain_factor) {
    uint16_t words[6] = {
        (uint16_t)index_offset, (uint16_t)learning_time_offset_h,
        (uint16_t)learning_time_gain_h, (uint16_t)gating_max_duration_min,
        (uint16_t)std_initial, (uint16_t)gain_factor,
    };
    if (!sensirion_write_cmd_words(handle->i2c_dev, 0x60E1, words, 6)) return false;
    vTaskDelay(pdMS_TO_TICKS(20));
    return true;
}

bool sen5x_get_nox_algorithm_tuning(sen5x_handle_t handle,
    int16_t *index_offset, int16_t *learning_time_offset_h, int16_t *learning_time_gain_h,
    int16_t *gating_max_duration_min, int16_t *std_initial, int16_t *gain_factor) {
    uint16_t words[6];
    if (!sensirion_cmd_read(handle->i2c_dev, 0x60E1, 20, words, 6)) return false;
    *index_offset            = (int16_t)words[0];
    *learning_time_offset_h  = (int16_t)words[1];
    *learning_time_gain_h    = (int16_t)words[2];
    *gating_max_duration_min = (int16_t)words[3];
    *std_initial             = (int16_t)words[4];
    *gain_factor             = (int16_t)words[5];
    return true;
}

bool sen5x_set_rht_acceleration_mode(sen5x_handle_t handle, uint16_t mode) {
    if (!sensirion_write_cmd_words(handle->i2c_dev, 0x60F7, &mode, 1)) return false;
    vTaskDelay(pdMS_TO_TICKS(20));
    return true;
}

bool sen5x_get_rht_acceleration_mode(sen5x_handle_t handle, uint16_t *mode) {
    return sensirion_cmd_read(handle->i2c_dev, 0x60F7, 20, mode, 1);
}

bool sen5x_set_voc_algorithm_state(sen5x_handle_t handle, const uint8_t state[8]) {
    if (!sensirion_write_cmd_bytes(handle->i2c_dev, 0x6181, state, 8)) return false;
    vTaskDelay(pdMS_TO_TICKS(20));
    return true;
}

bool sen5x_get_voc_algorithm_state(sen5x_handle_t handle, uint8_t state[8]) {
    if (!sensirion_write_cmd(handle->i2c_dev, 0x6181)) return false;
    vTaskDelay(pdMS_TO_TICKS(20));
    return sensirion_read_bytes(handle->i2c_dev, state, 8);
}

bool sen5x_start_fan_cleaning(sen5x_handle_t handle) {
    if (!sensirion_write_cmd(handle->i2c_dev, 0x5607)) return false;
    vTaskDelay(pdMS_TO_TICKS(20));
    return true;
}

bool sen5x_set_fan_auto_cleaning_interval(sen5x_handle_t handle, uint32_t interval_s) {
    uint16_t words[2] = { (uint16_t)(interval_s >> 16), (uint16_t)(interval_s & 0xFFFF) };
    if (!sensirion_write_cmd_words(handle->i2c_dev, 0x8004, words, 2)) return false;
    vTaskDelay(pdMS_TO_TICKS(20));
    return true;
}

bool sen5x_get_fan_auto_cleaning_interval(sen5x_handle_t handle, uint32_t *interval_s) {
    uint16_t words[2];
    if (!sensirion_cmd_read(handle->i2c_dev, 0x8004, 20, words, 2)) return false;
    *interval_s = ((uint32_t)words[0] << 16) | words[1];
    return true;
}

static bool read_ascii_string(sen5x_handle_t handle, uint16_t cmd, char *buf, size_t max_len) {
    // Wire format: 32 bytes of ASCII data with CRC per 2 bytes = 48 bytes on wire
    uint8_t raw[32];
    if (!sensirion_write_cmd(handle->i2c_dev, cmd)) return false;
    vTaskDelay(pdMS_TO_TICKS(50));
    if (!sensirion_read_bytes(handle->i2c_dev, raw, 32)) return false;
    size_t copy_len = max_len > 32 ? 32 : max_len - 1;
    memcpy(buf, raw, copy_len);
    buf[copy_len] = '\0';
    return true;
}

bool sen5x_get_product_name(sen5x_handle_t handle, char *name, size_t max_len) {
    return read_ascii_string(handle, 0xD014, name, max_len);
}

bool sen5x_get_serial_number(sen5x_handle_t handle, char *serial, size_t max_len) {
    return read_ascii_string(handle, 0xD033, serial, max_len);
}

bool sen5x_get_version(sen5x_handle_t handle,
    uint8_t *fw_major, uint8_t *fw_minor, bool *fw_debug,
    uint8_t *hw_major, uint8_t *hw_minor,
    uint8_t *proto_major, uint8_t *proto_minor) {
    uint16_t words[4];
    // Response layout per word: [fw_maj,fw_min] [fw_dbg,hw_maj] [hw_min,proto_maj] [proto_min,padding]
    if (!sensirion_cmd_read(handle->i2c_dev, 0xD100, 20, words, 4)) return false;
    if (fw_major)    *fw_major    = (uint8_t)(words[0] >> 8);
    if (fw_minor)    *fw_minor    = (uint8_t)(words[0] & 0xFF);
    if (fw_debug)    *fw_debug    = (words[1] >> 8) != 0;
    if (hw_major)    *hw_major    = (uint8_t)(words[1] & 0xFF);
    if (hw_minor)    *hw_minor    = (uint8_t)(words[2] >> 8);
    if (proto_major) *proto_major = (uint8_t)(words[2] & 0xFF);
    if (proto_minor) *proto_minor = (uint8_t)(words[3] >> 8);
    return true;
}

static bool read_device_status_cmd(sen5x_handle_t handle, uint16_t cmd, uint32_t *status) {
    uint16_t words[2];
    if (!sensirion_cmd_read(handle->i2c_dev, cmd, 20, words, 2)) return false;
    *status = ((uint32_t)words[0] << 16) | words[1];
    return true;
}

bool sen5x_read_device_status(sen5x_handle_t handle, uint32_t *status) {
    return read_device_status_cmd(handle, 0xD206, status);
}

bool sen5x_read_and_clear_device_status(sen5x_handle_t handle, uint32_t *status) {
    return read_device_status_cmd(handle, 0xD210, status);
}

bool sen5x_device_reset(sen5x_handle_t handle) {
    if (!sensirion_write_cmd(handle->i2c_dev, 0xD304)) return false;
    vTaskDelay(pdMS_TO_TICKS(200));
    return true;
}
