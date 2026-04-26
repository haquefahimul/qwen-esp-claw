/*
 * Nano-Claw Hook System Implementation
 * 
 * Copyright 2024 Nano-Claw Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cap_hooks.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "cap_hooks";

/**
 * @brief Hook system context
 */
typedef struct {
    hook_t hooks[HOOK_EVENT_MAX][HOOKS_MAX_PER_EVENT];
    SemaphoreHandle_t mutex;
    bool initialized;
} hooks_context_t;

static hooks_context_t s_hooks_ctx = {0};

// Comparison function for sorting hooks by priority
static int compare_hooks_by_priority(const void *a, const void *b) {
    const hook_t *hook_a = (const hook_t *)a;
    const hook_t *hook_b = (const hook_t *)b;
    
    if (hook_a->name[0] == '\0') return 1;  // Empty slots go to end
    if (hook_b->name[0] == '\0') return -1;
    
    return (int)hook_a->priority - (int)hook_b->priority;
}

esp_err_t hooks_init(void) {
    if (s_hooks_ctx.initialized) {
        ESP_LOGW(TAG, "Hook system already initialized");
        return ESP_OK;
    }
    
    memset(&s_hooks_ctx, 0, sizeof(hooks_context_t));
    
    s_hooks_ctx.mutex = xSemaphoreCreateMutex();
    if (!s_hooks_ctx.mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }
    
    s_hooks_ctx.initialized = true;
    ESP_LOGI(TAG, "Hook system initialized");
    
    return ESP_OK;
}

esp_err_t hooks_deinit(void) {
    if (!s_hooks_ctx.initialized) {
        return ESP_OK;
    }
    
    s_hooks_ctx.initialized = false;
    
    if (s_hooks_ctx.mutex) {
        vSemaphoreDelete(s_hooks_ctx.mutex);
        s_hooks_ctx.mutex = NULL;
    }
    
    ESP_LOGI(TAG, "Hook system deinitialized");
    return ESP_OK;
}

esp_err_t hook_register(hook_event_t event, const char *name,
                        hook_callback_t callback, hook_priority_t priority,
                        void *user_data) {
    if (!s_hooks_ctx.initialized || !name || !callback) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (event < 0 || event >= HOOK_EVENT_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_hooks_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Check if hook with same name exists
    for (int i = 0; i < HOOKS_MAX_PER_EVENT; i++) {
        if (strcmp(s_hooks_ctx.hooks[event][i].name, name) == 0) {
            xSemaphoreGive(s_hooks_ctx.mutex);
            ESP_LOGW(TAG, "Hook '%s' already registered", name);
            return ESP_ERR_INVALID_STATE;
        }
    }
    
    // Find empty slot
    int slot = -1;
    for (int i = 0; i < HOOKS_MAX_PER_EVENT; i++) {
        if (s_hooks_ctx.hooks[event][i].name[0] == '\0') {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        xSemaphoreGive(s_hooks_ctx.mutex);
        ESP_LOGW(TAG, "Max hooks reached for event %d (%d)", 
                 event, HOOKS_MAX_PER_EVENT);
        return ESP_ERR_NO_MEM;
    }
    
    // Setup hook
    hook_t *hook = &s_hooks_ctx.hooks[event][slot];
    strncpy(hook->name, name, HOOK_NAME_MAX - 1);
    hook->name[HOOK_NAME_MAX - 1] = '\0';
    hook->event = event;
    hook->callback = callback;
    hook->priority = priority;
    hook->user_data = user_data;
    hook->enabled = true;
    hook->execute_count = 0;
    hook->fail_count = 0;
    
    xSemaphoreGive(s_hooks_ctx.mutex);
    
    ESP_LOGI(TAG, "Registered hook '%s' for event %d (priority: %d)", 
             name, event, priority);
    return ESP_OK;
}

esp_err_t hook_unregister(const char *name) {
    if (!s_hooks_ctx.initialized || !name) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_hooks_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t ret = ESP_ERR_NOT_FOUND;
    
    for (int event = 0; event < HOOK_EVENT_MAX; event++) {
        for (int i = 0; i < HOOKS_MAX_PER_EVENT; i++) {
            if (strcmp(s_hooks_ctx.hooks[event][i].name, name) == 0) {
                s_hooks_ctx.hooks[event][i].name[0] = '\0';
                s_hooks_ctx.hooks[event][i].enabled = false;
                ret = ESP_OK;
                break;
            }
        }
        if (ret == ESP_OK) break;
    }
    
    xSemaphoreGive(s_hooks_ctx.mutex);
    return ret;
}

esp_err_t hook_unregister_all(hook_event_t event) {
    if (!s_hooks_ctx.initialized) {
        return ESP_OK;
    }
    
    if (event < 0 || event >= HOOK_EVENT_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_hooks_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    for (int i = 0; i < HOOKS_MAX_PER_EVENT; i++) {
        s_hooks_ctx.hooks[event][i].name[0] = '\0';
        s_hooks_ctx.hooks[event][i].enabled = false;
    }
    
    xSemaphoreGive(s_hooks_ctx.mutex);
    ESP_LOGI(TAG, "Unregistered all hooks for event %d", event);
    return ESP_OK;
}

esp_err_t hook_set_enabled(const char *name, bool enabled) {
    if (!s_hooks_ctx.initialized || !name) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_hooks_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t ret = ESP_ERR_NOT_FOUND;
    
    for (int event = 0; event < HOOK_EVENT_MAX; event++) {
        for (int i = 0; i < HOOKS_MAX_PER_EVENT; i++) {
            if (strcmp(s_hooks_ctx.hooks[event][i].name, name) == 0) {
                s_hooks_ctx.hooks[event][i].enabled = enabled;
                ret = ESP_OK;
                break;
            }
        }
        if (ret == ESP_OK) break;
    }
    
    xSemaphoreGive(s_hooks_ctx.mutex);
    return ret;
}

esp_err_t hooks_execute(hook_event_t event, void *data) {
    if (!s_hooks_ctx.initialized) {
        return ESP_OK;
    }
    
    if (event < 0 || event >= HOOK_EVENT_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_hooks_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Copy enabled hooks to temporary array for sorting
    hook_t temp_hooks[HOOKS_MAX_PER_EVENT];
    int count = 0;
    
    for (int i = 0; i < HOOKS_MAX_PER_EVENT; i++) {
        if (s_hooks_ctx.hooks[event][i].name[0] != '\0' &&
            s_hooks_ctx.hooks[event][i].enabled) {
            memcpy(&temp_hooks[count], &s_hooks_ctx.hooks[event][i], sizeof(hook_t));
            count++;
        }
    }
    
    xSemaphoreGive(s_hooks_ctx.mutex);
    
    if (count == 0) {
        return ESP_OK;
    }
    
    // Sort by priority
    qsort(temp_hooks, count, sizeof(hook_t), compare_hooks_by_priority);
    
    // Execute hooks in priority order
    esp_err_t result = ESP_OK;
    
    for (int i = 0; i < count; i++) {
        hook_t *hook = &temp_hooks[i];
        
        ESP_LOGD(TAG, "Executing hook '%s' (priority: %d)", 
                 hook->name, hook->priority);
        
        hook->execute_count++;
        
        esp_err_t ret = hook->callback(event, data, hook->user_data);
        
        if (ret != ESP_OK) {
            hook->fail_count++;
            ESP_LOGW(TAG, "Hook '%s' returned error: %d", hook->name, ret);
            
            if (ret == ESP_ERR_INVALID_STATE) {
                // Abort chain
                result = ESP_ERR_INVALID_STATE;
                break;
            }
        }
    }
    
    // Update fail counts in main array
    if (xSemaphoreTake(s_hooks_ctx.mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        for (int i = 0; i < count; i++) {
            for (int j = 0; j < HOOKS_MAX_PER_EVENT; j++) {
                if (strcmp(s_hooks_ctx.hooks[event][j].name, temp_hooks[i].name) == 0) {
                    s_hooks_ctx.hooks[event][j].fail_count = temp_hooks[i].fail_count;
                    s_hooks_ctx.hooks[event][j].execute_count = temp_hooks[i].execute_count;
                    break;
                }
            }
        }
        xSemaphoreGive(s_hooks_ctx.mutex);
    }
    
    return result;
}

esp_err_t hook_get(const char *name, hook_t *hook) {
    if (!s_hooks_ctx.initialized || !name) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_hooks_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t ret = ESP_ERR_NOT_FOUND;
    
    for (int event = 0; event < HOOK_EVENT_MAX; event++) {
        for (int i = 0; i < HOOKS_MAX_PER_EVENT; i++) {
            if (strcmp(s_hooks_ctx.hooks[event][i].name, name) == 0) {
                if (hook) {
                    memcpy(hook, &s_hooks_ctx.hooks[event][i], sizeof(hook_t));
                }
                ret = ESP_OK;
                break;
            }
        }
        if (ret == ESP_OK) break;
    }
    
    xSemaphoreGive(s_hooks_ctx.mutex);
    return ret;
}

uint32_t hooks_get_count(hook_event_t event) {
    if (!s_hooks_ctx.initialized || event < 0 || event >= HOOK_EVENT_MAX) {
        return 0;
    }
    
    uint32_t count = 0;
    
    if (xSemaphoreTake(s_hooks_ctx.mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        for (int i = 0; i < HOOKS_MAX_PER_EVENT; i++) {
            if (s_hooks_ctx.hooks[event][i].name[0] != '\0') {
                count++;
            }
        }
        xSemaphoreGive(s_hooks_ctx.mutex);
    }
    
    return count;
}

uint32_t hooks_get_total_count(void) {
    if (!s_hooks_ctx.initialized) {
        return 0;
    }
    
    uint32_t count = 0;
    
    if (xSemaphoreTake(s_hooks_ctx.mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        for (int event = 0; event < HOOK_EVENT_MAX; event++) {
            for (int i = 0; i < HOOKS_MAX_PER_EVENT; i++) {
                if (s_hooks_ctx.hooks[event][i].name[0] != '\0') {
                    count++;
                }
            }
        }
        xSemaphoreGive(s_hooks_ctx.mutex);
    }
    
    return count;
}

void hooks_get_stats(uint32_t *total, uint32_t *enabled, uint32_t *disabled) {
    uint32_t t = 0, e = 0, d = 0;
    
    if (s_hooks_ctx.initialized && 
        xSemaphoreTake(s_hooks_ctx.mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        
        for (int event = 0; event < HOOK_EVENT_MAX; event++) {
            for (int i = 0; i < HOOKS_MAX_PER_EVENT; i++) {
                if (s_hooks_ctx.hooks[event][i].name[0] != '\0') {
                    t++;
                    if (s_hooks_ctx.hooks[event][i].enabled) {
                        e++;
                    } else {
                        d++;
                    }
                }
            }
        }
        xSemaphoreGive(s_hooks_ctx.mutex);
    }
    
    if (total) *total = t;
    if (enabled) *enabled = e;
    if (disabled) *disabled = d;
}
