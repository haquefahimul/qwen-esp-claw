#include "cap_hooks.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "hooks";

typedef struct {
    hook_callback_t callback;
    hook_priority_t priority;
    bool enabled;
} hook_entry_t;

typedef struct {
    hook_entry_t entries[HOOK_MAX_CHAIN];
    int count;
} hook_chain_t;

static hook_chain_t chains[8];
static SemaphoreHandle_t hooks_mutex;

esp_err_t hooks_init(void) {
    memset(chains, 0, sizeof(chains));
    hooks_mutex = xSemaphoreCreateMutex();
    if (!hooks_mutex) return ESP_ERR_NO_MEM;
    ESP_LOGI(TAG, "Hooks system initialized");
    return ESP_OK;
}

esp_err_t hooks_register(hook_event_t event, hook_priority_t priority, hook_callback_t callback) {
    if (event >= 8 || !callback) return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(hooks_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
    
    hook_chain_t *chain = &chains[event];
    if (chain->count >= HOOK_MAX_CHAIN) {
        xSemaphoreGive(hooks_mutex);
        return ESP_ERR_NO_MEM;
    }
    
    // Insert sorted by priority
    int i;
    for (i = chain->count - 1; i >= 0 && chains[event].entries[i].priority > priority; i--) {
        chains[event].entries[i + 1] = chains[event].entries[i];
    }
    
    chains[event].entries[i + 1].callback = callback;
    chains[event].entries[i + 1].priority = priority;
    chains[event].entries[i + 1].enabled = true;
    chain->count++;
    
    xSemaphoreGive(hooks_mutex);
    ESP_LOGD(TAG, "Registered hook for event %d with priority %d", event, priority);
    return ESP_OK;
}

esp_err_t hooks_unregister(hook_event_t event, hook_callback_t callback) {
    if (event >= 8 || !callback) return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(hooks_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
    
    for (int i = 0; i < chains[event].count; i++) {
        if (chains[event].entries[i].callback == callback) {
            for (int j = i; j < chains[event].count - 1; j++) {
                chains[event].entries[j] = chains[event].entries[j + 1];
            }
            chains[event].count--;
            xSemaphoreGive(hooks_mutex);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(hooks_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t hooks_trigger(hook_event_t event, hook_data_t *data) {
    if (event >= 8) return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(hooks_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
    
    hook_chain_t *chain = &chains[event];
    for (int i = 0; i < chain->count; i++) {
        if (chain->entries[i].enabled && chain->entries[i].callback) {
            hook_result_t result = chain->entries[i].callback(event, data);
            if (result == HOOK_ABORT) {
                xSemaphoreGive(hooks_mutex);
                ESP_LOGD(TAG, "Hook aborted event %d", event);
                return ESP_OK;
            } else if (result == HOOK_SKIP_REMAINING) {
                break;
            }
        }
    }
    
    xSemaphoreGive(hooks_mutex);
    return ESP_OK;
}

int hooks_get_count(hook_event_t event) {
    if (event >= 8) return 0;
    return chains[event].count;
}
