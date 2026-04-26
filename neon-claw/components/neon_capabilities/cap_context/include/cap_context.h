/**
 * @file cap_context.h
 * @brief Context Engine for Neon-Claw
 * 
 * Features: Sliding window, Token counting, Memory management, Session history
 * Source: Ported from OpenClaw/femtoclaw
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Configuration constants
#define CAP_CONTEXT_MAX_ITEMS 32
#define CAP_CONTEXT_BUFFER_SIZE 1024

/**
 * @brief Initialize Context Engine
 * @return ESP_OK on success
 */
esp_err_t cap_context_init(void);

/**
 * @brief Deinitialize Context Engine
 * @return ESP_OK on success
 */
esp_err_t cap_context_deinit(void);

/**
 * @brief Main processing function
 * @param data Input data pointer
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t cap_context_process(const void* data, size_t len);

/**
 * @brief Get component status
 * @return true if active
 */
bool cap_context_is_active(void);

/**
 * @brief Get statistics
 * @param count Pointer to store item count
 * @return ESP_OK on success
 */
esp_err_t cap_context_get_stats(int* count);

#endif // _CAP_CONTEXT_H
