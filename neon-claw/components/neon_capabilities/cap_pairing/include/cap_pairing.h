/**
 * @file cap_pairing.h
 * @brief Device Pairing System for Neon-Claw
 * 
 * Features: 8-char codes, 5 channels, 1h TTL, JSON storage
 * Source: Ported from OpenClaw
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Configuration constants
#define CAP_PAIRING_MAX_ITEMS 32
#define CAP_PAIRING_BUFFER_SIZE 1024

/**
 * @brief Initialize Device Pairing System
 * @return ESP_OK on success
 */
esp_err_t cap_pairing_init(void);

/**
 * @brief Deinitialize Device Pairing System
 * @return ESP_OK on success
 */
esp_err_t cap_pairing_deinit(void);

/**
 * @brief Main processing function
 * @param data Input data pointer
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t cap_pairing_process(const void* data, size_t len);

/**
 * @brief Get component status
 * @return true if active
 */
bool cap_pairing_is_active(void);

/**
 * @brief Get statistics
 * @param count Pointer to store item count
 * @return ESP_OK on success
 */
esp_err_t cap_pairing_get_stats(int* count);

#endif // _CAP_PAIRING_H
