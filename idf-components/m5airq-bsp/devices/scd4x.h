/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Hiroki Kawakami
 */

#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCD40_I2C_ADDR_62 0x62
#define SCD41_I2C_ADDR_62 0x62
#define SCD43_I2C_ADDR_62 0x62

typedef struct scd4x *scd4x_handle_t;

typedef enum {
    SCD4X_SENSOR_VARIANT_MASK = 0x00F000,
    SCD4X_SENSOR_VARIANT_SCD40 = 0x000000,
    SCD4X_SENSOR_VARIANT_SCD41 = 0x001000,
    SCD4X_SENSOR_VARIANT_SCD42 = 0x002000,
    SCD4X_SENSOR_VARIANT_SCD43 = 0x005000,
} scd4x_sensor_variant_t;

scd4x_handle_t scd4x_init(i2c_master_bus_handle_t i2c_bus, uint8_t i2c_address);
void scd4x_deinit(scd4x_handle_t handle);

// Periodic measurement
bool scd4x_start_periodic_measurement(scd4x_handle_t handle);
bool scd4x_stop_periodic_measurement(scd4x_handle_t handle);
bool scd4x_start_low_power_periodic_measurement(scd4x_handle_t handle);
bool scd4x_read_measurement(scd4x_handle_t handle, uint16_t *co2_concentration, float *temperature, float *relative_humidity);
bool scd4x_get_data_ready_status(scd4x_handle_t handle, bool *data_ready_status);

// Configuration (idle mode only)
bool scd4x_set_temperature_offset(scd4x_handle_t handle, float temperature_offset);
bool scd4x_get_temperature_offset(scd4x_handle_t handle, float *temperature_offset);
bool scd4x_set_sensor_altitude(scd4x_handle_t handle, uint16_t altitude_m);
bool scd4x_get_sensor_altitude(scd4x_handle_t handle, uint16_t *altitude_m);
bool scd4x_set_ambient_pressure(scd4x_handle_t handle, float ambient_pressure_pa);
bool scd4x_get_ambient_pressure(scd4x_handle_t handle, float *ambient_pressure_pa);

// Automatic self-calibration (idle mode only)
bool scd4x_set_automatic_self_calibration_enabled(scd4x_handle_t handle, bool enabled);
bool scd4x_get_automatic_self_calibration_enabled(scd4x_handle_t handle, bool *enabled);
bool scd4x_set_automatic_self_calibration_target(scd4x_handle_t handle, uint16_t target_co2_ppm);
bool scd4x_get_automatic_self_calibration_target(scd4x_handle_t handle, uint16_t *target_co2_ppm);
bool scd4x_set_automatic_self_calibration_initial_period(scd4x_handle_t handle, uint16_t hours);
bool scd4x_get_automatic_self_calibration_initial_period(scd4x_handle_t handle, uint16_t *hours);
bool scd4x_set_automatic_self_calibration_standard_period(scd4x_handle_t handle, uint16_t hours);
bool scd4x_get_automatic_self_calibration_standard_period(scd4x_handle_t handle, uint16_t *hours);

// Forced recalibration (idle mode only)
bool scd4x_perform_forced_recalibration(scd4x_handle_t handle, uint16_t target_co2_ppm, uint16_t *frc_correction);

// Maintenance (idle mode only)
bool scd4x_persist_settings(scd4x_handle_t handle);
bool scd4x_get_serial_number(scd4x_handle_t handle, uint64_t *serial_number);
bool scd4x_perform_self_test(scd4x_handle_t handle, uint16_t *sensor_status);
bool scd4x_perform_factory_reset(scd4x_handle_t handle);
bool scd4x_reinit(scd4x_handle_t handle);
bool scd4x_get_sensor_variant(scd4x_handle_t handle, scd4x_sensor_variant_t *sensor_variant);

// SCD41 only
bool scd4x_measure_single_shot(scd4x_handle_t handle);
bool scd4x_measure_single_shot_rht_only(scd4x_handle_t handle);
bool scd4x_power_down(scd4x_handle_t handle);
bool scd4x_wake_up(scd4x_handle_t handle);

#ifdef __cplusplus
}
#endif
