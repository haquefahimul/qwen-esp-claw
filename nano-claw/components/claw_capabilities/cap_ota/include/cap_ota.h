/**
 * @file cap_ota.h
 * @brief Secure HTTPS OTA Update Manager for Nano-Claw
 * 
 * Handles secure firmware updates over HTTPS with SHA256 verification.
 * Supports rollback on failed boots.
 */

#ifndef CAP_OTA_H
#define CAP_OTA_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OTA Event types
 */
typedef enum {
    OTA_EVENT_START,
    OTA_EVENT_PROGRESS,
    OTA_EVENT_COMPLETE,
    OTA_EVENT_ERROR,
    OTA_EVENT_ROLLBACK
} ota_event_t;

/**
 * @brief OTA Progress callback
 * @param event Event type
 * @param progress Percent complete (0-100)
 * @param user_data User context
 */
typedef void (*ota_progress_cb_t)(ota_event_t event, int progress, void* user_data);

/**
 * @brief Initialize OTA subsystem
 * @return ESP_OK on success
 */
esp_err_t ota_init(void);

/**
 * @brief Request an OTA update
 * @param url HTTPS URL to firmware binary
 * @param expected_sha256 Expected SHA256 hash (64 hex chars) or NULL to skip
 * @return ESP_OK if update started, error otherwise
 */
esp_err_t ota_request_update(const char* url, const char* expected_sha256);

/**
 * @brief Check if update is available (non-blocking)
 * @param url HTTPS URL to check
 * @param out_available Pointer to store availability flag
 * @return ESP_OK on success
 */
esp_err_t ota_check_update(const char* url, bool* out_available);

/**
 * @brief Set progress callback
 * @param cb Callback function
 * @param user_data User context passed to callback
 */
void ota_set_callback(ota_progress_cb_t cb, void* user_data);

/**
 * @brief Get current OTA state
 * @return true if update in progress
 */
bool ota_is_updating(void);

/**
 * @brief Mark boot successful (call after successful app startup)
 */
void ota_mark_boot_successful(void);

/**
 * @brief Request rollback to previous firmware
 * @return ESP_OK on success
 */
esp_err_t ota_request_rollback(void);

#ifdef __cplusplus
}
#endif

#endif // CAP_OTA_H
