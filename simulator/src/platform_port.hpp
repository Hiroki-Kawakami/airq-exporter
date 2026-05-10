#pragma once
#include "bsp_mock/bsp_mock.hpp"
#include <cstdio>
#include <cstdarg>

#define LOG_E(tag, fmt, ...) printf("[E] %s: " fmt "\n", tag, ##__VA_ARGS__)
#define LOG_W(tag, fmt, ...) printf("[W] %s: " fmt "\n", tag, ##__VA_ARGS__)
#define LOG_I(tag, fmt, ...) printf("[I] %s: " fmt "\n", tag, ##__VA_ARGS__)
#define LOG_D(tag, fmt, ...) do {} while (0)

inline void epd_refresh(bool full_refresh = false) {}
