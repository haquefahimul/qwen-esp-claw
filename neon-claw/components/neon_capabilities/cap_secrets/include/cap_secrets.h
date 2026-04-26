/**
 * @file cap_secrets.h
 * @brief Encrypted Secrets Vault for Neon-Claw
 * 
 * Features: AES-256-GCM, 5 secret types, Rotation, Secure wipe
 * Source: Ported from OpenClaw
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Configuration constants
#define CAP_SECRETS_MAX_ITEMS 32
#define CAP_SECRETS_BUFFER_SIZE 1024

/**
 * @brief Initialize Encrypted Secrets Vault
 * @return ESP_OK on success
 */
esp_err_t cap_secrets_init(void);

/**
 * @brief Deinitialize Encrypted Secrets Vault
 * @return ESP_OK on success
 */
esp_err_t cap_secrets_deinit(void);

/**
 * @brief Main processing function
 * @param data Input data pointer
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t cap_secrets_process(const void* data, size_t len);

/**
 * @brief Get component status
 * @return true if active
 */
bool cap_secrets_is_active(void);

/**
 * @brief Get statistics
 * @param count Pointer to store item count
 * @return ESP_OK on success
 */
esp_err_t cap_secrets_get_stats(int* count);

#endif // _CAP_SECRETS_H
