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

#define SENSIRION_I2C_TIMEOUT_MS 100

uint8_t sensirion_crc8(const uint8_t *data, size_t len);

// Write command (2 bytes) only
bool sensirion_write_cmd(i2c_master_dev_handle_t dev, uint16_t cmd);

// Write command + N uint16 words (each word followed by CRC)
bool sensirion_write_cmd_words(i2c_master_dev_handle_t dev, uint16_t cmd, const uint16_t *words, size_t n);

// Write command + N bytes (each pair of bytes followed by CRC, n must be even)
bool sensirion_write_cmd_bytes(i2c_master_dev_handle_t dev, uint16_t cmd, const uint8_t *data, size_t n);

// Read N uint16 words with CRC verification
bool sensirion_read_words(i2c_master_dev_handle_t dev, uint16_t *words, size_t count);

// Read N bytes with CRC verification (n must be even, wire format: [b0, b1, crc] per pair)
bool sensirion_read_bytes(i2c_master_dev_handle_t dev, uint8_t *data, size_t n);

// Write command, wait delay_ms, then read N words
bool sensirion_cmd_read(i2c_master_dev_handle_t dev, uint16_t cmd, uint32_t delay_ms, uint16_t *words, size_t count);

// Write command + N words, wait delay_ms, then read M words
bool sensirion_cmd_write_read(i2c_master_dev_handle_t dev, uint16_t cmd,
                              const uint16_t *wwords, size_t wcount,
                              uint32_t delay_ms,
                              uint16_t *rwords, size_t rcount);

#ifdef __cplusplus
}
#endif
