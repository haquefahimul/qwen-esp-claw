/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "claw_event_router.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pairing request structure
 */
typedef struct {
    char id[32];           /*!< Unique pairing request ID */
    char code[16];         /*!< Human-readable pairing code (8 chars) */
    int64_t created_at;    /*!< Creation timestamp (ms since epoch) */
    int64_t last_seen_at;  /*!< Last activity timestamp */
    char meta[256];        /*!< Optional metadata (JSON string) */
    bool active;           /*!< Whether the pairing is still active */
} cap_pairing_request_t;

/**
 * @brief Pairing channel types supported
 */
typedef enum {
    CAP_PAIRING_CHANNEL_NONE = 0,
    CAP_PAIRING_CHANNEL_DISCORD,
    CAP_PAIRING_CHANNEL_TELEGRAM,
    CAP_PAIRING_CHANNEL_WECHAT,
    CAP_PAIRING_CHANNEL_QQ,
    CAP_PAIRING_CHANNEL_FEISHU,
    CAP_PAIRING_CHANNEL_MAX
} cap_pairing_channel_t;

/**
 * @brief Initialize the pairing manager
 * 
 * @param pairing_root_dir Root directory for storing pairing data
 * @return ESP_OK on success, appropriate error code otherwise
 */
esp_err_t cap_pairing_init(const char *pairing_root_dir);

/**
 * @brief Deinitialize the pairing manager
 * 
 * @return ESP_OK on success
 */
esp_err_t cap_pairing_deinit(void);

/**
 * @brief Generate a new pairing code
 * 
 * @param channel Channel type
 * @param out_code Buffer to store the generated code (min 16 bytes)
 * @param out_code_size Size of output buffer
 * @return ESP_OK on success
 */
esp_err_t cap_pairing_generate_code(cap_pairing_channel_t channel, char *out_code, size_t out_code_size);

/**
 * @brief Create a new pairing request
 * 
 * @param channel Channel type
 * @param code The pairing code
 * @param meta Optional metadata (can be NULL)
 * @param out_request_id Buffer to store the generated request ID
 * @param out_request_id_size Size of output buffer
 * @return ESP_OK on success
 */
esp_err_t cap_pairing_create_request(cap_pairing_channel_t channel, const char *code, 
                                     const char *meta, char *out_request_id, size_t out_request_id_size);

/**
 * @brief Get a pairing request by ID
 * 
 * @param request_id The request ID to look up
 * @param out_request Pointer to store the request data
 * @return ESP_OK if found, ESP_ERR_NOT_FOUND if not found
 */
esp_err_t cap_pairing_get_request(const char *request_id, cap_pairing_request_t *out_request);

/**
 * @brief Update last seen timestamp for a pairing request
 * 
 * @param request_id The request ID
 * @return ESP_OK on success
 */
esp_err_t cap_pairing_touch_request(const char *request_id);

/**
 * @brief Complete/accept a pairing request
 * 
 * @param request_id The request ID
 * @param user_id User ID that accepted the pairing
 * @return ESP_OK on success
 */
esp_err_t cap_pairing_accept_request(const char *request_id, const char *user_id);

/**
 * @brief Reject/cancel a pairing request
 * 
 * @param request_id The request ID
 * @return ESP_OK on success
 */
esp_err_t cap_pairing_reject_request(const char *request_id);

/**
 * @brief List all active pairing requests for a channel
 * 
 * @param channel Channel type
 * @param out_requests Array to store request pointers
 * @param max_requests Maximum number of requests to return
 * @param out_count Actual number of requests returned
 * @return ESP_OK on success
 */
esp_err_t cap_pairing_list_active(cap_pairing_channel_t channel, 
                                  cap_pairing_request_t **out_requests,
                                  size_t max_requests, size_t *out_count);

/**
 * @brief Prune expired pairing requests
 * 
 * @return Number of pruned requests
 */
size_t cap_pairing_prune_expired(void);

/**
 * @brief Validate a pairing code
 * 
 * @param code The code to validate
 * @return true if valid format, false otherwise
 */
bool cap_pairing_validate_code_format(const char *code);

/**
 * @brief Register the pairing capability group
 * 
 * @return ESP_OK on success
 */
esp_err_t cap_pairing_register_group(void);

/**
 * @brief Get the pairing storage path for a channel
 * 
 * @param channel Channel type
 * @param buf Buffer to store the path
 * @param buf_size Size of buffer
 * @return ESP_OK on success
 */
esp_err_t cap_pairing_get_storage_path(cap_pairing_channel_t channel, char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif
