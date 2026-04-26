/**
 * @file cap_watchdog.h
 * @brief Watchdog & Safe Mode for Neon-Claw
 * 
 * Features: Task monitoring, Stack analysis, Auto-reboot, Crash recovery
 * Source: Ported from zclaw
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Configuration constants
#define CAP_WATCHDOG_MAX_ITEMS 32
#define CAP_WATCHDOG_BUFFER_SIZE 1024

/**
 * @brief Initialize Watchdog & Safe Mode
 * @return ESP_OK on success
 */
esp_err_t cap_watchdog_init(void);

/**
 * @brief Deinitialize Watchdog & Safe Mode
 * @return ESP_OK on success
 */
esp_err_t cap_watchdog_deinit(void);

/**
 * @brief Main processing function
 * @param data Input data pointer
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t cap_watchdog_process(const void* data, size_t len);

/**
 * @brief Get component status
 * @return true if active
 */
bool cap_watchdog_is_active(void);

/**
 * @brief Get statistics
 * @param count Pointer to store item count
 * @return ESP_OK on success
 */
esp_err_t cap_watchdog_get_stats(int* count);

#endif // _CAP_WATCHDOG_H
