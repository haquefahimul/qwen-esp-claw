/**
 * @file cap_ratelimit.h
 * @brief Rate Limiter for Neon-Claw
 * 
 * Features: Sliding window, Burst allowance, Per-user limits, Global limits
 * Source: Ported from zclaw/OpenClaw
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Configuration constants
#define CAP_RATELIMIT_MAX_ITEMS 32
#define CAP_RATELIMIT_BUFFER_SIZE 1024

/**
 * @brief Initialize Rate Limiter
 * @return ESP_OK on success
 */
esp_err_t cap_ratelimit_init(void);

/**
 * @brief Deinitialize Rate Limiter
 * @return ESP_OK on success
 */
esp_err_t cap_ratelimit_deinit(void);

/**
 * @brief Main processing function
 * @param data Input data pointer
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t cap_ratelimit_process(const void* data, size_t len);

/**
 * @brief Get component status
 * @return true if active
 */
bool cap_ratelimit_is_active(void);

/**
 * @brief Get statistics
 * @param count Pointer to store item count
 * @return ESP_OK on success
 */
esp_err_t cap_ratelimit_get_stats(int* count);

#endif // _CAP_RATELIMIT_H
