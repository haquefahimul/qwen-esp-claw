/*
 * Nano-Claw Secrets Vault Implementation
 * Copyright 2024 Nano-Claw Contributors
 */

#include "cap_secrets.h"
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "cap_secrets";

#define MAX_SECRETS 32

typedef struct {
    secret_entry_t entries[MAX_SECRETS];
    SemaphoreHandle_t mutex;
    bool initialized;
} secrets_ctx_t;

static secrets_ctx_t s_sec_ctx = {0};

// Stub for encryption - use ESP32 HMAC in production
static void encrypt_value(const char *in, char *out) {
    // Simple XOR stub - replace with AES-256-GCM
    strcpy(out, in);
}

static void decrypt_value(const char *in, char *out) {
    strcpy(out, in);
}

esp_err_t secrets_init(void) {
    if (s_sec_ctx.initialized) return ESP_OK;
    
    memset(&s_sec_ctx, 0, sizeof(secrets_ctx_t));
    s_sec_ctx.mutex = xSemaphoreCreateMutex();
    if (!s_sec_ctx.mutex) return ESP_ERR_NO_MEM;
    
    s_sec_ctx.initialized = true;
    ESP_LOGI(TAG, "Secrets vault initialized");
    return ESP_OK;
}

esp_err_t secrets_set(const char *name, const char *value, secret_type_t type) {
    if (!s_sec_ctx.initialized || !name || !value) return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(s_sec_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
    
    uint32_t now = esp_timer_get_time() / 1000;
    int slot = -1;
    
    // Check if exists
    for (int i = 0; i < MAX_SECRETS; i++) {
        if (strcmp(s_sec_ctx.entries[i].name, name) == 0) {
            slot = i;
            break;
        }
    }
    
    // Find empty slot if not exists
    if (slot == -1) {
        for (int i = 0; i < MAX_SECRETS; i++) {
            if (!s_sec_ctx.entries[i].enabled && s_sec_ctx.entries[i].name[0] == '\0') {
                slot = i;
                break;
            }
        }
    }
    
    if (slot == -1) {
        xSemaphoreGive(s_sec_ctx.mutex);
        return ESP_ERR_NO_MEM;
    }
    
    secret_entry_t *entry = &s_sec_ctx.entries[slot];
    strncpy(entry->name, name, SECRET_NAME_MAX - 1);
    entry->type = type;
    encrypt_value(value, entry->value);
    entry->created_at = (entry->created_at == 0) ? now : entry->created_at;
    entry->updated_at = now;
    entry->access_count = 0;
    entry->enabled = true;
    
    xSemaphoreGive(s_sec_ctx.mutex);
    ESP_LOGI(TAG, "Secret '%s' stored", name);
    return ESP_OK;
}

esp_err_t secrets_get(const char *name, char *value_out, size_t max_len) {
    if (!s_sec_ctx.initialized || !name || !value_out) return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(s_sec_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
    
    esp_err_t ret = ESP_ERR_NOT_FOUND;
    for (int i = 0; i < MAX_SECRETS; i++) {
        if (strcmp(s_sec_ctx.entries[i].name, name) == 0 && 
            s_sec_ctx.entries[i].enabled) {
            decrypt_value(s_sec_ctx.entries[i].value, value_out);
            value_out[max_len - 1] = '\0';
            s_sec_ctx.entries[i].access_count++;
            ret = ESP_OK;
            break;
        }
    }
    
    xSemaphoreGive(s_sec_ctx.mutex);
    return ret;
}

esp_err_t secrets_delete(const char *name) {
    if (!s_sec_ctx.initialized || !name) return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(s_sec_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
    
    esp_err_t ret = ESP_ERR_NOT_FOUND;
    for (int i = 0; i < MAX_SECRETS; i++) {
        if (strcmp(s_sec_ctx.entries[i].name, name) == 0) {
            memset(&s_sec_ctx.entries[i], 0, sizeof(secret_entry_t));
            ret = ESP_OK;
            break;
        }
    }
    
    xSemaphoreGive(s_sec_ctx.mutex);
    return ret;
}

esp_err_t secrets_rotate(const char *name, const char *new_value) {
    return secrets_set(name, new_value, SECRET_TYPE_STRING);
}

esp_err_t secrets_wipe(void) {
    if (!s_sec_ctx.initialized) return ESP_OK;
    
    if (xSemaphoreTake(s_sec_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
    
    for (int i = 0; i < MAX_SECRETS; i++) {
        memset(&s_sec_ctx.entries[i], 0, sizeof(secret_entry_t));
    }
    
    xSemaphoreGive(s_sec_ctx.mutex);
    ESP_LOGI(TAG, "All secrets wiped");
    return ESP_OK;
}

uint32_t secrets_count(void) {
    if (!s_sec_ctx.initialized) return 0;
    
    uint32_t count = 0;
    if (xSemaphoreTake(s_sec_ctx.mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        for (int i = 0; i < MAX_SECRETS; i++) {
            if (s_sec_ctx.entries[i].enabled) count++;
        }
        xSemaphoreGive(s_sec_ctx.mutex);
    }
    return count;
}
