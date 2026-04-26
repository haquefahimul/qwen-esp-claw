/**
 * @file cap_react.h
 * @brief ReAct Agent Engine for Neon-Claw
 * 
 * Features: Reason+Act pattern, Tool registry, Pattern matching, Multi-step
 * Source: Ported from femtoclaw
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Configuration constants
#define CAP_REACT_MAX_ITEMS 32
#define CAP_REACT_BUFFER_SIZE 1024

/**
 * @brief Initialize ReAct Agent Engine
 * @return ESP_OK on success
 */
esp_err_t cap_react_init(void);

/**
 * @brief Deinitialize ReAct Agent Engine
 * @return ESP_OK on success
 */
esp_err_t cap_react_deinit(void);

/**
 * @brief Main processing function
 * @param data Input data pointer
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t cap_react_process(const void* data, size_t len);

/**
 * @brief Get component status
 * @return true if active
 */
bool cap_react_is_active(void);

/**
 * @brief Get statistics
 * @param count Pointer to store item count
 * @return ESP_OK on success
 */
esp_err_t cap_react_get_stats(int* count);

#endif // _CAP_REACT_H
