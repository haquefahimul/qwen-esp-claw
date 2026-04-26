/**
 * @file cap_context.c
 * @brief Context Window Management Implementation
 */

#include "cap_context.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_heap_caps.h"

static const char* TAG = "cap_context";

// Default token counter: ~4 chars per token (rough estimation)
static size_t default_token_counter(const char* content) {
    if (!content) return 0;
    return strlen(content) / 4;
}

static token_counter_func_t g_token_counter = default_token_counter;

context_manager_t* ctx_mgr_init(const context_config_t* config) {
    if (!config) {
        ESP_LOGE(TAG, "Config is NULL");
        return NULL;
    }

    context_manager_t* mgr = heap_caps_malloc(sizeof(context_manager_t), MALLOC_CAP_SPIRAM);
    if (!mgr) {
        ESP_LOGE(TAG, "Failed to allocate manager");
        return NULL;
    }
    memset(mgr, 0, sizeof(context_manager_t));

    mgr->config = *config;
    mgr->capacity = config->max_messages > 0 ? config->max_messages : 32;
    mgr->messages = heap_caps_calloc(mgr->capacity, sizeof(context_message_t), MALLOC_CAP_SPIRAM);
    if (!mgr->messages) {
        ESP_LOGE(TAG, "Failed to allocate messages array");
        free(mgr);
        return NULL;
    }

    mgr->mutex = xSemaphoreCreateMutex();
    if (!mgr->mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        free(mgr->messages);
        free(mgr);
        return NULL;
    }

    mgr->initialized = true;
    ESP_LOGI(TAG, "Context manager initialized: max_tokens=%zu, max_messages=%zu", 
             config->max_tokens, config->max_messages);
    return mgr;
}

void ctx_mgr_deinit(context_manager_t* mgr) {
    if (!mgr) return;

    if (mgr->mutex) {
        xSemaphoreTake(mgr->mutex, portMAX_DELAY);
    }

    for (size_t i = 0; i < mgr->message_count; i++) {
        if (mgr->messages[i].content) {
            free(mgr->messages[i].content);
        }
    }

    if (mgr->messages) free(mgr->messages);
    if (mgr->mutex) vSemaphoreDelete(mgr->mutex);
    free(mgr);

    ESP_LOGI(TAG, "Context manager deinitialized");
}

esp_err_t ctx_mgr_add_message(context_manager_t* mgr, 
                              context_role_t role,
                              const char* content,
                              context_priority_t priority) {
    if (!mgr || !content) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(mgr->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Check if compression needed
    size_t new_tokens = g_token_counter(content);
    if (mgr->total_tokens + new_tokens > mgr->config.max_tokens) {
        if (mgr->config.enable_auto_compress) {
            size_t target = mgr->config.max_tokens * 0.7;
            ctx_mgr_compress(mgr, target);
        } else {
            ESP_LOGW(TAG, "Context full, message rejected");
            xSemaphoreGive(mgr->mutex);
            return ESP_ERR_NO_MEM;
        }
    }

    // Make room if at capacity
    if (mgr->message_count >= mgr->capacity) {
        ctx_mgr_remove_oldest(mgr);
    }

    // Add message
    context_message_t* msg = &mgr->messages[mgr->message_count];
    msg->role = role;
    msg->priority = priority;
    msg->token_count = new_tokens;
    msg->timestamp = xTaskGetTickCount();
    msg->is_compressed = false;

    msg->content = strdup(content);
    if (!msg->content) {
        xSemaphoreGive(mgr->mutex);
        return ESP_ERR_NO_MEM;
    }

    mgr->message_count++;
    mgr->total_tokens += new_tokens;

    ESP_LOGD(TAG, "Added message: role=%d, tokens=%zu, total=%zu", 
             role, new_tokens, mgr->total_tokens);
    
    xSemaphoreGive(mgr->mutex);
    return ESP_OK;
}

size_t ctx_mgr_get_token_count(context_manager_t* mgr) {
    if (!mgr) return 0;
    return mgr->total_tokens;
}

size_t ctx_mgr_get_message_count(context_manager_t* mgr) {
    if (!mgr) return 0;
    return mgr->message_count;
}

esp_err_t ctx_mgr_compress(context_manager_t* mgr, size_t target_tokens) {
    if (!mgr) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(mgr->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    if (mgr->total_tokens <= target_tokens) {
        xSemaphoreGive(mgr->mutex);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Compressing context: %zu -> %zu tokens", mgr->total_tokens, target_tokens);

    // Remove low-priority messages first
    for (int priority = CONTEXT_PRIORITY_LOW; priority < CONTEXT_PRIORITY_CRITICAL && mgr->total_tokens > target_tokens; priority++) {
        for (int i = 0; i < mgr->message_count && mgr->total_tokens > target_tokens; i++) {
            if (mgr->messages[i].priority == priority) {
                ESP_LOGD(TAG, "Removing message %d (priority=%d, tokens=%zu)", 
                         i, priority, mgr->messages[i].token_count);
                
                mgr->total_tokens -= mgr->messages[i].token_count;
                free(mgr->messages[i].content);
                
                // Shift remaining messages
                memmove(&mgr->messages[i], &mgr->messages[i+1], 
                        (mgr->message_count - i - 1) * sizeof(context_message_t));
                mgr->message_count--;
                i--; // Recheck this index
            }
        }
    }

    xSemaphoreGive(mgr->mutex);
    ESP_LOGI(TAG, "Compression complete: %zu tokens, %zu messages", 
             mgr->total_tokens, mgr->message_count);
    return ESP_OK;
}

void ctx_mgr_set_token_counter(context_manager_t* mgr, token_counter_func_t counter_func) {
    if (counter_func) {
        g_token_counter = counter_func;
    }
}

esp_err_t ctx_mgr_get_formatted_prompt(context_manager_t* mgr, char* buffer, size_t buffer_len) {
    if (!mgr || !buffer) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(mgr->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    size_t offset = 0;
    for (size_t i = 0; i < mgr->message_count && offset < buffer_len - 100; i++) {
        const char* role_str = "";
        switch (mgr->messages[i].role) {
            case CONTEXT_ROLE_SYSTEM: role_str = "system"; break;
            case CONTEXT_ROLE_USER: role_str = "user"; break;
            case CONTEXT_ROLE_ASSISTANT: role_str = "assistant"; break;
            case CONTEXT_ROLE_TOOL: role_str = "tool"; break;
        }

        int written = snprintf(buffer + offset, buffer_len - offset,
                               "<|im_start|>%s\n%s<|im_end|>\n",
                               role_str, mgr->messages[i].content);
        if (written > 0) offset += written;
    }

    xSemaphoreGive(mgr->mutex);
    return ESP_OK;
}

esp_err_t ctx_mgr_clear(context_manager_t* mgr) {
    if (!mgr) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(mgr->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    for (size_t i = 0; i < mgr->message_count; i++) {
        if (mgr->messages[i].content) {
            free(mgr->messages[i].content);
            mgr->messages[i].content = NULL;
        }
    }

    mgr->message_count = 0;
    mgr->total_tokens = 0;

    xSemaphoreGive(mgr->mutex);
    ESP_LOGD(TAG, "Context cleared");
    return ESP_OK;
}

esp_err_t ctx_mgr_remove_oldest(context_manager_t* mgr) {
    if (!mgr || mgr->message_count == 0) return ESP_ERR_INVALID_STATE;

    // Find oldest non-critical message
    int oldest_idx = -1;
    TickType_t oldest_time = portMAX_DELAY;

    for (int i = 0; i < mgr->message_count; i++) {
        if (mgr->messages[i].priority < CONTEXT_PRIORITY_CRITICAL &&
            mgr->messages[i].timestamp < oldest_time) {
            oldest_time = mgr->messages[i].timestamp;
            oldest_idx = i;
        }
    }

    if (oldest_idx == -1) {
        ESP_LOGW(TAG, "No removable messages (all critical)");
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGD(TAG, "Removing oldest message at index %d", oldest_idx);
    mgr->total_tokens -= mgr->messages[oldest_idx].token_count;
    free(mgr->messages[oldest_idx].content);

    // Shift remaining
    memmove(&mgr->messages[oldest_idx], &mgr->messages[oldest_idx+1],
            (mgr->message_count - oldest_idx - 1) * sizeof(context_message_t));
    mgr->message_count--;

    return ESP_OK;
}

void ctx_mgr_get_memory_stats(context_manager_t* mgr, size_t* heap_size, size_t* psram_size) {
    if (!mgr) {
        if (heap_size) *heap_size = 0;
        if (psram_size) *psram_size = 0;
        return;
    }

    size_t heap = sizeof(context_manager_t) + (mgr->capacity * sizeof(context_message_t));
    size_t psram = 0;

    for (size_t i = 0; i < mgr->message_count; i++) {
        if (mgr->messages[i].content) {
            psram += strlen(mgr->messages[i].content) + 1;
        }
    }

    if (heap_size) *heap_size = heap;
    if (psram_size) *psram_size = psram;
}
