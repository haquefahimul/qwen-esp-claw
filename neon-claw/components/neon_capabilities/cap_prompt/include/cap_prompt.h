/**
 * @file cap_prompt.h
 * @brief Prompt Template Engine for Neon-Claw
 * 
 * Features: Mustache syntax, Variable substitution, Conditional blocks, Loop support
 * Source: Ported from zeroclaw/OpenClaw
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Configuration constants
#define CAP_PROMPT_MAX_ITEMS 32
#define CAP_PROMPT_BUFFER_SIZE 1024

/**
 * @brief Initialize Prompt Template Engine
 * @return ESP_OK on success
 */
esp_err_t cap_prompt_init(void);

/**
 * @brief Deinitialize Prompt Template Engine
 * @return ESP_OK on success
 */
esp_err_t cap_prompt_deinit(void);

/**
 * @brief Main processing function
 * @param data Input data pointer
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t cap_prompt_process(const void* data, size_t len);

/**
 * @brief Get component status
 * @return true if active
 */
bool cap_prompt_is_active(void);

/**
 * @brief Get statistics
 * @param count Pointer to store item count
 * @return ESP_OK on success
 */
esp_err_t cap_prompt_get_stats(int* count);

#endif // _CAP_PROMPT_H
