/*
 * Nano-Claw Hook System Capability
 * 
 * Implements extensible event hooks for ESP32-S3
 * Features: Pre/post hooks, priority levels, chaining
 * 
 * Copyright 2024 Nano-Claw Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CAP_HOOKS_H
#define CAP_HOOKS_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of hooks per event type
 */
#define HOOKS_MAX_PER_EVENT CONFIG_CLAW_HOOKS_MAX_PER_EVENT

/**
 * @brief Maximum hook name length
 */
#define HOOK_NAME_MAX 32

/**
 * @brief Hook event types
 */
typedef enum {
    HOOK_PRE_REQUEST,     ///< Before LLM request
    HOOK_POST_REQUEST,    ///< After LLM request sent
    HOOK_PRE_RESPONSE,    ///< Before processing response
    HOOK_POST_RESPONSE,   ///< After response processed
    HOOK_ON_ERROR,        ///< On error occurrence
    HOOK_ON_CONNECT,      ///< On connection established
    HOOK_ON_DISCONNECT,   ///< On connection lost
    HOOK_CUSTOM           ///< Custom user-defined hook
} hook_event_t;

/**
 * @brief Hook priority levels
 */
typedef enum {
    HOOK_PRIORITY_CRITICAL = 0,  ///< Execute first
    HOOK_PRIORITY_HIGH = 10,     ///< High priority
    HOOK_PRIORITY_NORMAL = 50,   ///< Normal priority
    HOOK_PRIORITY_LOW = 100,     ///< Low priority (execute last)
    HOOK_PRIORITY_BACKGROUND = 200  ///< Background tasks
} hook_priority_t;

/**
 * @brief Hook callback function type
 * 
 * @param event The event that triggered the hook
 * @param data Event-specific data (type depends on event)
 * @param user_data User-defined data passed during registration
 * @return ESP_OK to continue, ESP_ERR_INVALID_STATE to abort chain
 */
typedef esp_err_t (*hook_callback_t)(hook_event_t event, void *data, 
                                     void *user_data);

/**
 * @brief Hook structure
 */
typedef struct {
    char name[HOOK_NAME_MAX];       ///< Hook name
    hook_event_t event;             ///< Event type
    hook_callback_t callback;       ///< Callback function
    hook_priority_t priority;       ///< Execution priority
    void *user_data;                ///< User data
    bool enabled;                   ///< Hook enabled flag
    uint32_t execute_count;         ///< Number of times executed
    uint32_t fail_count;            ///< Number of failures
} hook_t;

/**
 * @brief Initialize hook system
 * 
 * @return ESP_OK on success
 */
esp_err_t hooks_init(void);

/**
 * @brief Deinitialize hook system
 * 
 * @return ESP_OK on success
 */
esp_err_t hooks_deinit(void);

/**
 * @brief Register a hook
 * 
 * @param event Event type to hook into
 * @param name Unique hook name
 * @param callback Callback function
 * @param priority Execution priority
 * @param user_data User data for callback
 * @return ESP_OK on success, ESP_ERR_NO_MEM if max hooks reached
 */
esp_err_t hook_register(hook_event_t event, const char *name,
                        hook_callback_t callback, hook_priority_t priority,
                        void *user_data);

/**
 * @brief Unregister a hook by name
 * 
 * @param name Hook name
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if not found
 */
esp_err_t hook_unregister(const char *name);

/**
 * @brief Unregister all hooks for an event
 * 
 * @param event Event type
 * @return ESP_OK on success
 */
esp_err_t hook_unregister_all(hook_event_t event);

/**
 * @brief Enable/disable a hook
 * 
 * @param name Hook name
 * @param enabled true to enable, false to disable
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if not found
 */
esp_err_t hook_set_enabled(const char *name, bool enabled);

/**
 * @brief Execute all hooks for an event
 * 
 * Hooks are executed in priority order (lowest number first).
 * If any hook returns ESP_ERR_INVALID_STATE, execution stops.
 * 
 * @param event Event type
 * @param data Event-specific data
 * @return ESP_OK if all hooks succeeded or not aborted,
 *         ESP_ERR_INVALID_STATE if aborted by hook
 */
esp_err_t hooks_execute(hook_event_t event, void *data);

/**
 * @brief Get hook by name
 * 
 * @param name Hook name
 * @param hook Output hook structure (can be NULL)
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if not found
 */
esp_err_t hook_get(const char *name, hook_t *hook);

/**
 * @brief Get number of hooks for an event
 * 
 * @param event Event type
 * @return Number of registered hooks
 */
uint32_t hooks_get_count(hook_event_t event);

/**
 * @brief Get total number of hooks
 * 
 * @return Total number of registered hooks
 */
uint32_t hooks_get_total_count(void);

/**
 * @brief Get hook statistics
 * 
 * @param total Total hooks
 * @param enabled Enabled hooks
 * @param disabled Disabled hooks
 */
void hooks_get_stats(uint32_t *total, uint32_t *enabled, uint32_t *disabled);

#ifdef __cplusplus
}
#endif

#endif // CAP_HOOKS_H
