/**
 * @file cap_heartbeat.h
 * @brief Heartbeat Monitor for Neon-Claw
 * 
 * Features: Periodic check-ins, 1s-24h intervals, Auto-reconnect, Channel health
 * Source: Ported from OpenClaw
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Configuration constants
#define CAP_HEARTBEAT_MAX_ITEMS 32
#define CAP_HEARTBEAT_BUFFER_SIZE 1024

/**
 * @brief Initialize Heartbeat Monitor
 * @return ESP_OK on success
 */
esp_err_t cap_heartbeat_init(void);

/**
 * @brief Deinitialize Heartbeat Monitor
 * @return ESP_OK on success
 */
esp_err_t cap_heartbeat_deinit(void);

/**
 * @brief Main processing function
 * @param data Input data pointer
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t cap_heartbeat_process(const void* data, size_t len);

/**
 * @brief Get component status
 * @return true if active
 */
bool cap_heartbeat_is_active(void);

/**
 * @brief Get statistics
 * @param count Pointer to store item count
 * @return ESP_OK on success
 */
esp_err_t cap_heartbeat_get_stats(int* count);

#endif // _CAP_HEARTBEAT_H
