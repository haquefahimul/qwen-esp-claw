/**
 * @file cap_vector.h
 * @brief Vector Store Lite for Neon-Claw
 * 
 * Features: In-PSRAM index, Cosine similarity, Top-K search, Embedding cache
 * Source: Ported from mimiclaw/zeroclaw
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Configuration constants
#define CAP_VECTOR_MAX_ITEMS 32
#define CAP_VECTOR_BUFFER_SIZE 1024

/**
 * @brief Initialize Vector Store Lite
 * @return ESP_OK on success
 */
esp_err_t cap_vector_init(void);

/**
 * @brief Deinitialize Vector Store Lite
 * @return ESP_OK on success
 */
esp_err_t cap_vector_deinit(void);

/**
 * @brief Main processing function
 * @param data Input data pointer
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t cap_vector_process(const void* data, size_t len);

/**
 * @brief Get component status
 * @return true if active
 */
bool cap_vector_is_active(void);

/**
 * @brief Get statistics
 * @param count Pointer to store item count
 * @return ESP_OK on success
 */
esp_err_t cap_vector_get_stats(int* count);

#endif // _CAP_VECTOR_H
