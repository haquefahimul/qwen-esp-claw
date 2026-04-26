/**
 * @file cap_storage.h
 * @brief Type-Safe NVS Wrapper for Nano-Claw
 * 
 * Provides a simplified, type-safe interface to ESP-IDF NVS storage.
 * Supports automatic namespace management and blob storage.
 */

#ifndef CAP_STORAGE_H
#define CAP_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Storage data types
 */
typedef enum {
    STORAGE_TYPE_I8,
    STORAGE_TYPE_I16,
    STORAGE_TYPE_I32,
    STORAGE_TYPE_U8,
    STORAGE_TYPE_U16,
    STORAGE_TYPE_U32,
    STORAGE_TYPE_STR,
    STORAGE_TYPE_BLOB
} storage_type_t;

/**
 * @brief Initialize storage subsystem
 * @return ESP_OK on success
 */
esp_err_t storage_init(void);

/**
 * @brief Deinitialize storage subsystem
 */
void storage_deinit(void);

/**
 * @brief Save an integer value
 * @param key Key name (max 15 chars)
 * @param value Value to store
 * @return ESP_OK on success
 */
esp_err_t storage_save_int(const char* key, int32_t value);

/**
 * @brief Load an integer value
 * @param key Key name
 * @param out_value Pointer to store result
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if key missing
 */
esp_err_t storage_load_int(const char* key, int32_t* out_value);

/**
 * @brief Save a string value
 * @param key Key name
 * @param value String to store
 * @return ESP_OK on success
 */
esp_err_t storage_save_str(const char* key, const char* value);

/**
 * @brief Load a string value
 * @param key Key name
 * @param out_buffer Buffer to store result
 * @param buffer_size Size of output buffer
 * @return ESP_OK on success
 */
esp_err_t storage_load_str(const char* key, char* out_buffer, size_t buffer_size);

/**
 * @brief Save a blob (binary data)
 * @param key Key name
 * @param data Pointer to binary data
 * @param data_len Length of data
 * @return ESP_OK on success
 */
esp_err_t storage_save_blob(const char* key, const void* data, size_t data_len);

/**
 * @brief Load a blob
 * @param key Key name
 * @param out_buffer Buffer to store result
 * @param buffer_size Size of output buffer
 * @param out_len Pointer to store actual length read
 * @return ESP_OK on success
 */
esp_err_t storage_load_blob(const char* key, void* out_buffer, size_t buffer_size, size_t* out_len);

/**
 * @brief Delete a key
 * @param key Key name
 * @return ESP_OK on success
 */
esp_err_t storage_delete(const char* key);

/**
 * @brief Erase all data in namespace
 * @return ESP_OK on success
 */
esp_err_t storage_erase_all(void);

#ifdef __cplusplus
}
#endif

#endif // CAP_STORAGE_H
