/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum secret name length
 */
#define CAP_SECRETS_MAX_NAME_LEN    64

/**
 * @brief Maximum secret value length  
 */
#define CAP_SECRETS_MAX_VALUE_LEN   512

/**
 * @brief Maximum number of secrets supported
 */
#define CAP_SECRETS_MAX_COUNT       32

/**
 * @brief Secret types
 */
typedef enum {
    CAP_SECRETS_TYPE_STRING = 0,   /*!< Plain string secret */
    CAP_SECRETS_TYPE_API_KEY,      /*!< API key (masked in logs) */
    CAP_SECRETS_TYPE_TOKEN,        /*!< OAuth/access token */
    CAP_SECRETS_TYPE_PASSWORD,     /*!< Password (never logged) */
    CAP_SECRETS_TYPE_CERTIFICATE,  /*!< Certificate or private key */
} cap_secrets_type_t;

/**
 * @brief Secret metadata
 */
typedef struct {
    char name[CAP_SECRETS_MAX_NAME_LEN];    /*!< Secret name */
    cap_secrets_type_t type;                /*!< Secret type */
    int64_t created_at;                     /*!< Creation timestamp */
    int64_t updated_at;                     /*!< Last update timestamp */
    bool encrypted;                         /*!< Whether stored encrypted */
    uint8_t access_count;                   /*!< Number of times accessed */
} cap_secrets_meta_t;

/**
 * @brief Initialize the secrets vault
 * 
 * @param vault_root_dir Root directory for storing encrypted secrets
 * @param master_key Optional master encryption key (can use HW encryption if NULL)
 * @param master_key_len Length of master key
 * @return ESP_OK on success
 */
esp_err_t cap_secrets_init(const char *vault_root_dir, const uint8_t *master_key, size_t master_key_len);

/**
 * @brief Deinitialize the secrets vault
 * 
 * @return ESP_OK on success
 */
esp_err_t cap_secrets_deinit(void);

/**
 * @brief Store a secret
 * 
 * @param name Secret name (unique identifier)
 * @param value Secret value
 * @param type Secret type
 * @return ESP_OK on success
 */
esp_err_t cap_secrets_set(const char *name, const char *value, cap_secrets_type_t type);

/**
 * @brief Retrieve a secret value
 * 
 * @param name Secret name
 * @param out_value Buffer to store the value
 * @param out_value_size Size of output buffer
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if not found
 */
esp_err_t cap_secrets_get(const char *name, char *out_value, size_t out_value_size);

/**
 * @brief Delete a secret
 * 
 * @param name Secret name
 * @return ESP_OK on success
 */
esp_err_t cap_secrets_delete(const char *name);

/**
 * @brief List all secret names
 * 
 * @param out_names Array of strings to store names
 * @param max_names Maximum number of names to return
 * @param out_count Actual number returned
 * @return ESP_OK on success
 */
esp_err_t cap_secrets_list(char out_names[][CAP_SECRETS_MAX_NAME_LEN], 
                           size_t max_names, size_t *out_count);

/**
 * @brief Get secret metadata
 * 
 * @param name Secret name
 * @param out_meta Pointer to store metadata
 * @return ESP_OK on success
 */
esp_err_t cap_secrets_get_meta(const char *name, cap_secrets_meta_t *out_meta);

/**
 * @brief Check if a secret exists
 * 
 * @param name Secret name
 * @return true if exists
 */
bool cap_secrets_exists(const char *name);

/**
 * @brief Export secrets (encrypted backup)
 * 
 * @param export_path Path to export file
 * @param export_key Encryption key for export
 * @param export_key_len Length of export key
 * @return ESP_OK on success
 */
esp_err_t cap_secrets_export(const char *export_path, const uint8_t *export_key, size_t export_key_len);

/**
 * @brief Import secrets from encrypted backup
 * 
 * @param import_path Path to import file
 * @param import_key Decryption key
 * @param import_key_len Length of import key
 * @return ESP_OK on success
 */
esp_err_t cap_secrets_import(const char *import_path, const uint8_t *import_key, size_t import_key_len);

/**
 * @brief Rotate/refresh a secret
 * 
 * @param name Secret name
 * @param new_value New secret value
 * @return ESP_OK on success
 */
esp_err_t cap_secrets_rotate(const char *name, const char *new_value);

/**
 * @brief Get count of stored secrets
 * 
 * @return Number of secrets
 */
size_t cap_secrets_count(void);

/**
 * @brief Clear all secrets (secure wipe)
 * 
 * @return ESP_OK on success
 */
esp_err_t cap_secrets_clear_all(void);

/**
 * @brief Register the secrets capability group
 * 
 * @return ESP_OK on success
 */
esp_err_t cap_secrets_register_group(void);

#ifdef __cplusplus
}
#endif
