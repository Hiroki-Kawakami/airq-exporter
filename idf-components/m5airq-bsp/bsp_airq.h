/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Hiroki Kawakami
 */

#pragma once
#include "devices/scd4x.h"

#ifdef __cplusplus
extern "C" {
#endif

extern scd4x_handle_t bsp_airq_scd4x;

esp_err_t bsp_airq_init(void);

#ifdef __cplusplus
}
#endif
