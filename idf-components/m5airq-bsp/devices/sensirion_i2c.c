/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Hiroki Kawakami
 */

#include "sensirion_i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

uint8_t sensirion_crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

bool sensirion_write_cmd(i2c_master_dev_handle_t dev, uint16_t cmd) {
    uint8_t buf[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };
    return i2c_master_transmit(dev, buf, 2, pdMS_TO_TICKS(SENSIRION_I2C_TIMEOUT_MS)) == ESP_OK;
}

bool sensirion_write_cmd_words(i2c_master_dev_handle_t dev, uint16_t cmd, const uint16_t *words, size_t n) {
    // cmd(2) + max 6 words × 3 bytes = 20, 32 is generous
    uint8_t buf[32];
    buf[0] = (uint8_t)(cmd >> 8);
    buf[1] = (uint8_t)(cmd & 0xFF);
    for (size_t i = 0; i < n; i++) {
        buf[2 + i * 3 + 0] = (uint8_t)(words[i] >> 8);
        buf[2 + i * 3 + 1] = (uint8_t)(words[i] & 0xFF);
        buf[2 + i * 3 + 2] = sensirion_crc8(&buf[2 + i * 3], 2);
    }
    return i2c_master_transmit(dev, buf, 2 + n * 3, pdMS_TO_TICKS(SENSIRION_I2C_TIMEOUT_MS)) == ESP_OK;
}

bool sensirion_write_cmd_bytes(i2c_master_dev_handle_t dev, uint16_t cmd, const uint8_t *data, size_t n) {
    // n must be even; cmd(2) + max 8 bytes with CRC = 14 bytes
    uint8_t buf[32];
    size_t n_pairs = n / 2;
    buf[0] = (uint8_t)(cmd >> 8);
    buf[1] = (uint8_t)(cmd & 0xFF);
    for (size_t i = 0; i < n_pairs; i++) {
        buf[2 + i * 3 + 0] = data[i * 2];
        buf[2 + i * 3 + 1] = data[i * 2 + 1];
        buf[2 + i * 3 + 2] = sensirion_crc8(&buf[2 + i * 3], 2);
    }
    return i2c_master_transmit(dev, buf, 2 + n_pairs * 3, pdMS_TO_TICKS(SENSIRION_I2C_TIMEOUT_MS)) == ESP_OK;
}

bool sensirion_read_words(i2c_master_dev_handle_t dev, uint16_t *words, size_t count) {
    uint8_t rx[30]; // max 10 words × 3 bytes = 30
    if (i2c_master_receive(dev, rx, count * 3, pdMS_TO_TICKS(SENSIRION_I2C_TIMEOUT_MS)) != ESP_OK) {
        return false;
    }
    for (size_t i = 0; i < count; i++) {
        uint8_t *p = &rx[i * 3];
        if (sensirion_crc8(p, 2) != p[2]) return false;
        words[i] = ((uint16_t)p[0] << 8) | p[1];
    }
    return true;
}

bool sensirion_read_bytes(i2c_master_dev_handle_t dev, uint8_t *data, size_t n) {
    // n must be even; max 32 bytes = 16 pairs × 3 = 48 bytes on wire
    uint8_t rx[48];
    size_t n_pairs = n / 2;
    if (i2c_master_receive(dev, rx, n_pairs * 3, pdMS_TO_TICKS(SENSIRION_I2C_TIMEOUT_MS)) != ESP_OK) {
        return false;
    }
    for (size_t i = 0; i < n_pairs; i++) {
        uint8_t *p = &rx[i * 3];
        if (sensirion_crc8(p, 2) != p[2]) return false;
        data[i * 2]     = p[0];
        data[i * 2 + 1] = p[1];
    }
    return true;
}

bool sensirion_cmd_read(i2c_master_dev_handle_t dev, uint16_t cmd, uint32_t delay_ms, uint16_t *words, size_t count) {
    if (!sensirion_write_cmd(dev, cmd)) return false;
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    return sensirion_read_words(dev, words, count);
}

bool sensirion_cmd_write_read(i2c_master_dev_handle_t dev, uint16_t cmd,
                              const uint16_t *wwords, size_t wcount,
                              uint32_t delay_ms,
                              uint16_t *rwords, size_t rcount) {
    if (!sensirion_write_cmd_words(dev, cmd, wwords, wcount)) return false;
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    return sensirion_read_words(dev, rwords, rcount);
}
