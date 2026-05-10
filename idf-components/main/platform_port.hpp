#pragma once
#include "bsp_airq.h"
#include "esp_log.h"

#define LOG_E ESP_LOGE
#define LOG_W ESP_LOGW
#define LOG_I ESP_LOGI
#define LOG_D ESP_LOGD

void epd_refresh(bool full_refresh = false);
