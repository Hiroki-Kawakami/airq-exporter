#include "bsp_mock.hpp"
#include <cmath>
#include <ctime>
#include <algorithm>

// -------------------------------------------------------------------
// Temporal continuity: all values are pure functions of Unix time.
// No RNG state — same wall-clock time always produces the same output,
// regardless of when the process started.
//
// Noise: time is divided into 60-second slots. Within each slot the
// value is smoothly interpolated (smoothstep) between two adjacent
// hash values so changes are continuous. Amplitudes are exaggerated
// compared to reality for easier visual verification.
// -------------------------------------------------------------------

static constexpr double PI = 3.14159265358979323846;
static constexpr int SLOT_SEC = 60;

static uint32_t hash32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x45d9f3b;
    x ^= x >> 16;
    return x;
}

// Returns a value in [-1, 1] that varies smoothly with time.
// channel differentiates independent noise streams.
static float smooth_noise(double t, uint32_t channel) {
    int64_t slot = (int64_t)(t / SLOT_SEC);
    float frac   = (float)(t / SLOT_SEC - (double)slot);

    uint32_t h0 = hash32((uint32_t)(slot)       ^ (channel * 2654435761u));
    uint32_t h1 = hash32((uint32_t)(slot + 1)   ^ (channel * 2654435761u));

    float n0 = (float)(h0 & 0xFFFF) / 32767.5f - 1.0f;
    float n1 = (float)(h1 & 0xFFFF) / 32767.5f - 1.0f;

    float s = frac * frac * (3.0f - 2.0f * frac);  // smoothstep
    return n0 + s * (n1 - n0);
}

static float clampf(float x, float lo, float hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

struct SensorValues {
    uint16_t co2;
    float temperature, humidity;
    float pm1p0, pm2p5, pm4p0, pm10p0;
    float voc_index, nox_index;
};

static SensorValues compute() {
    time_t now   = time(nullptr);
    struct tm *lt = localtime(&now);
    double t     = (double)now;

    double hour       = lt->tm_hour + lt->tm_min / 60.0 + lt->tm_sec / 3600.0;
    int    day_of_year = lt->tm_yday;  // 0-364

    // ---- Temperature (°C) ----
    // Tokyo: annual mean ~15.5°C, amplitude ±12°C, peak ~day 213 (Aug 1)
    float temp_seasonal = 15.5f + 12.0f * (float)sin(2.0 * PI * (day_of_year - 80) / 365.25);
    // Diurnal: peak at 14h, ±5°C
    float temp_diurnal  =  5.0f * (float)sin(2.0 * PI * (hour - 8.0) / 24.0);
    // Noise: ±4°C (exaggerated for visibility)
    float temperature   = temp_seasonal + temp_diurnal + 4.0f * smooth_noise(t, 0);

    // ---- Humidity (%) ----
    // Seasonal: dry winter ~40%, humid summer ~80%; peak ~day 213
    float hum_seasonal = 60.0f + 20.0f * (float)sin(2.0 * PI * (day_of_year - 190) / 365.25);
    // Anti-correlated with temperature diurnal
    float hum_diurnal  = -5.0f * (float)sin(2.0 * PI * (hour - 8.0) / 24.0);
    float humidity     = clampf(hum_seasonal + hum_diurnal + 15.0f * smooth_noise(t, 1), 20.0f, 95.0f);

    // ---- CO2 (ppm) ----
    // Rises during daytime activity; base ~450 ppm (outdoor-ish)
    float co2_activity = 280.0f * (float)fmax(0.0, sin(2.0 * PI * (hour - 6.0) / 24.0));
    float co2_f        = 450.0f + co2_activity + 200.0f * smooth_noise(t, 2);
    uint16_t co2       = (uint16_t)fmax(380.0f, co2_f);

    // ---- PM2.5 (μg/m³) ----
    // Spring peak (kosa season ~March-May, day 60-150)
    float pm_spring = 15.0f * (float)fmax(0.0, cos(2.0 * PI * (day_of_year - 105) / 90.0));
    if (day_of_year < 60 || day_of_year > 150) pm_spring = 0.0f;
    // Rush hour bump
    float pm_rush = 6.0f * (float)fmax(0.0, -cos(2.0 * PI * hour / 24.0));
    // Noise: strictly positive component
    float pm_noise = 12.0f * (smooth_noise(t, 3) * 0.5f + 0.5f);
    float pm2p5    = fmaxf(0.5f, 5.0f + pm_spring + pm_rush + pm_noise);
    float pm1p0    = fmaxf(0.0f, pm2p5 * 0.65f + 2.0f * smooth_noise(t, 4));
    float pm4p0    = fmaxf(0.0f, pm2p5 * 1.25f + 3.0f * (smooth_noise(t, 5) * 0.5f + 0.5f));
    float pm10p0   = fmaxf(0.0f, pm2p5 * 1.70f + 5.0f * (smooth_noise(t, 6) * 0.5f + 0.5f));

    // ---- VOC index (Sensirion scale: 1-500, ~100 = clean air) ----
    // Higher in summer (plant VOCs), spike at cooking time (~18h)
    float voc_seasonal = 20.0f * (float)sin(2.0 * PI * (day_of_year - 150) / 365.25);
    float voc_cooking  = 55.0f * (float)fmax(0.0, sin(2.0 * PI * (hour - 17.0) / 24.0));
    float voc_index    = clampf(110.0f + voc_seasonal + voc_cooking + 45.0f * smooth_noise(t, 7),
                                1.0f, 500.0f);

    // ---- NOx index (Sensirion scale: 1-500, ~1 = clean air) ----
    // Peaks at morning and evening rush hours
    float nox_rush  = 6.0f * (float)fmax(0.0, sin(2.0 * PI * (hour - 8.0) / 12.0));
    float nox_index = clampf(2.0f + nox_rush + 4.0f * smooth_noise(t, 8), 1.0f, 50.0f);

    return {co2, temperature, humidity, pm1p0, pm2p5, pm4p0, pm10p0, voc_index, nox_index};
}

// -------------------------------------------------------------------
// Handle instances (the actual pointer value is never dereferenced)
// -------------------------------------------------------------------
struct scd4x_mock {};
struct sen5x_mock {};

static scd4x_mock scd4x_instance;
static sen5x_mock sen5x_instance;

scd4x_handle_t bsp_airq_scd4x = &scd4x_instance;
sen5x_handle_t bsp_airq_sen5x = &sen5x_instance;

// -------------------------------------------------------------------
// SCD4x mock
// -------------------------------------------------------------------
bool scd4x_start_periodic_measurement(scd4x_handle_t) { return true; }

bool scd4x_get_data_ready_status(scd4x_handle_t, bool *data_ready_status) {
    *data_ready_status = true;
    return true;
}

bool scd4x_read_measurement(scd4x_handle_t, uint16_t *co2, float *temperature, float *humidity) {
    auto v       = compute();
    *co2         = v.co2;
    *temperature = v.temperature;
    *humidity    = v.humidity;
    return true;
}

// -------------------------------------------------------------------
// SEN5x mock
// -------------------------------------------------------------------
bool sen5x_start_measurement(sen5x_handle_t) { return true; }

bool sen5x_read_data_ready(sen5x_handle_t, bool *data_ready) {
    *data_ready = true;
    return true;
}

bool sen5x_read_measured_values(sen5x_handle_t,
    float *pm1p0, float *pm2p5, float *pm4p0, float *pm10p0,
    float *humidity, float *temperature, float *voc_index, float *nox_index) {
    auto v = compute();
    if (pm1p0)       *pm1p0       = v.pm1p0;
    if (pm2p5)       *pm2p5       = v.pm2p5;
    if (pm4p0)       *pm4p0       = v.pm4p0;
    if (pm10p0)      *pm10p0      = v.pm10p0;
    if (humidity)    *humidity    = v.humidity;
    if (temperature) *temperature = v.temperature;
    if (voc_index)   *voc_index   = v.voc_index;
    if (nox_index)   *nox_index   = v.nox_index;
    return true;
}
