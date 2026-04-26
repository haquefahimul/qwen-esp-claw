/**
 * @file cap_eventbus.h
 * @brief Event Bus Pub-Sub for Neon-Claw
 * 
 * Features: Decoupled messaging, Topic-based, Async delivery, Queue management
 * Source: Ported from zeroclaw
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Configuration constants
#define CAP_EVENTBUS_MAX_ITEMS 32
#define CAP_EVENTBUS_BUFFER_SIZE 1024

/**
 * @brief Initialize Event Bus Pub-Sub
 * @return ESP_OK on success
 */
esp_err_t cap_eventbus_init(void);

/**
 * @brief Deinitialize Event Bus Pub-Sub
 * @return ESP_OK on success
 */
esp_err_t cap_eventbus_deinit(void);

/**
 * @brief Main processing function
 * @param data Input data pointer
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t cap_eventbus_process(const void* data, size_t len);

/**
 * @brief Get component status
 * @return true if active
 */
bool cap_eventbus_is_active(void);

/**
 * @brief Get statistics
 * @param count Pointer to store item count
 * @return ESP_OK on success
 */
esp_err_t cap_eventbus_get_stats(int* count);

#endif // _CAP_EVENTBUS_H
