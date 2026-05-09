/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Hiroki Kawakami
 */

#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SEN5X_I2C_ADDR 0x69

typedef struct sen5x *sen5x_handle_t;

sen5x_handle_t sen5x_init(i2c_master_bus_handle_t i2c_bus, uint8_t i2c_address);
void sen5x_deinit(sen5x_handle_t handle);

// Measurement control
bool sen5x_start_measurement(sen5x_handle_t handle);
bool sen5x_start_measurement_without_pm(sen5x_handle_t handle);  // SEN54, SEN55 only
bool sen5x_stop_measurement(sen5x_handle_t handle);
bool sen5x_read_data_ready(sen5x_handle_t handle, bool *data_ready);

// Read all measured values (NAN if unavailable)
bool sen5x_read_measured_values(sen5x_handle_t handle,
    float *pm1p0, float *pm2p5, float *pm4p0, float *pm10p0,
    float *humidity, float *temperature, float *voc_index, float *nox_index);

// Read extended PM values including number concentrations and typical particle size
bool sen5x_read_measured_pm_values(sen5x_handle_t handle,
    float *pm1p0, float *pm2p5, float *pm4p0, float *pm10p0,
    float *n0p5, float *n1p0, float *n2p5, float *n4p0, float *n10p0,
    float *typical_particle_size_um);

// Temperature offset (SEN54, SEN55 only)
bool sen5x_set_temperature_offset(sen5x_handle_t handle, float offset_celsius);
bool sen5x_get_temperature_offset(sen5x_handle_t handle, float *offset_celsius);
bool sen5x_set_temperature_offset_params(sen5x_handle_t handle, int16_t offset, int16_t slope, uint16_t time_constant);
bool sen5x_get_temperature_offset_params(sen5x_handle_t handle, int16_t *offset, int16_t *slope, uint16_t *time_constant);

// Warm start (SEN54, SEN55 only)
bool sen5x_set_warm_start_parameter(sen5x_handle_t handle, uint16_t warm_start);
bool sen5x_get_warm_start_parameter(sen5x_handle_t handle, uint16_t *warm_start);

// VOC algorithm tuning (SEN54, SEN55 only, idle mode only)
bool sen5x_set_voc_algorithm_tuning(sen5x_handle_t handle,
    int16_t index_offset, int16_t learning_time_offset_h, int16_t learning_time_gain_h,
    int16_t gating_max_duration_min, int16_t std_initial, int16_t gain_factor);
bool sen5x_get_voc_algorithm_tuning(sen5x_handle_t handle,
    int16_t *index_offset, int16_t *learning_time_offset_h, int16_t *learning_time_gain_h,
    int16_t *gating_max_duration_min, int16_t *std_initial, int16_t *gain_factor);

// NOx algorithm tuning (SEN55 only, idle mode only)
bool sen5x_set_nox_algorithm_tuning(sen5x_handle_t handle,
    int16_t index_offset, int16_t learning_time_offset_h, int16_t learning_time_gain_h,
    int16_t gating_max_duration_min, int16_t std_initial, int16_t gain_factor);
bool sen5x_get_nox_algorithm_tuning(sen5x_handle_t handle,
    int16_t *index_offset, int16_t *learning_time_offset_h, int16_t *learning_time_gain_h,
    int16_t *gating_max_duration_min, int16_t *std_initial, int16_t *gain_factor);

// RH/T acceleration mode (SEN54, SEN55 only)
bool sen5x_set_rht_acceleration_mode(sen5x_handle_t handle, uint16_t mode);
bool sen5x_get_rht_acceleration_mode(sen5x_handle_t handle, uint16_t *mode);

// VOC algorithm state (SEN54, SEN55 only)
bool sen5x_set_voc_algorithm_state(sen5x_handle_t handle, const uint8_t state[8]);
bool sen5x_get_voc_algorithm_state(sen5x_handle_t handle, uint8_t state[8]);

// Fan control
bool sen5x_start_fan_cleaning(sen5x_handle_t handle);
bool sen5x_set_fan_auto_cleaning_interval(sen5x_handle_t handle, uint32_t interval_s);
bool sen5x_get_fan_auto_cleaning_interval(sen5x_handle_t handle, uint32_t *interval_s);

// Device info
bool sen5x_get_product_name(sen5x_handle_t handle, char *name, size_t max_len);
bool sen5x_get_serial_number(sen5x_handle_t handle, char *serial, size_t max_len);
bool sen5x_get_version(sen5x_handle_t handle,
    uint8_t *fw_major, uint8_t *fw_minor, bool *fw_debug,
    uint8_t *hw_major, uint8_t *hw_minor,
    uint8_t *proto_major, uint8_t *proto_minor);

// Device status
bool sen5x_read_device_status(sen5x_handle_t handle, uint32_t *status);
bool sen5x_read_and_clear_device_status(sen5x_handle_t handle, uint32_t *status);
bool sen5x_device_reset(sen5x_handle_t handle);

#ifdef __cplusplus
}
#endif
