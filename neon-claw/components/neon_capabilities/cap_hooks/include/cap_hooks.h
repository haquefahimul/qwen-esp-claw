/**
 * @file cap_hooks.h
 * @brief Event Hook System for Neon-Claw
 * 
 * Features: 8 event types, 5 priority levels, Chainable hooks, Abort capability
 * Source: Ported from OpenClaw
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Configuration constants
#define CAP_HOOKS_MAX_ITEMS 32
#define CAP_HOOKS_BUFFER_SIZE 1024

/**
 * @brief Initialize Event Hook System
 * @return ESP_OK on success
 */
esp_err_t cap_hooks_init(void);

/**
 * @brief Deinitialize Event Hook System
 * @return ESP_OK on success
 */
esp_err_t cap_hooks_deinit(void);

/**
 * @brief Main processing function
 * @param data Input data pointer
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t cap_hooks_process(const void* data, size_t len);

/**
 * @brief Get component status
 * @return true if active
 */
bool cap_hooks_is_active(void);

/**
 * @brief Get statistics
 * @param count Pointer to store item count
 * @return ESP_OK on success
 */
esp_err_t cap_hooks_get_stats(int* count);

#endif // _CAP_HOOKS_H
