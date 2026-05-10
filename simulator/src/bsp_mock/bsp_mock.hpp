#pragma once
#include <cstdint>

struct scd4x_mock;
struct sen5x_mock;

typedef scd4x_mock *scd4x_handle_t;
typedef sen5x_mock *sen5x_handle_t;

extern scd4x_handle_t bsp_airq_scd4x;
extern sen5x_handle_t bsp_airq_sen5x;

bool scd4x_start_periodic_measurement(scd4x_handle_t handle);
bool scd4x_get_data_ready_status(scd4x_handle_t handle, bool *data_ready_status);
bool scd4x_read_measurement(scd4x_handle_t handle, uint16_t *co2, float *temperature, float *humidity);

bool sen5x_start_measurement(sen5x_handle_t handle);
bool sen5x_read_data_ready(sen5x_handle_t handle, bool *data_ready);
bool sen5x_read_measured_values(sen5x_handle_t handle,
    float *pm1p0, float *pm2p5, float *pm4p0, float *pm10p0,
    float *humidity, float *temperature, float *voc_index, float *nox_index);
