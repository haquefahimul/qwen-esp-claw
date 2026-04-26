/*
 * Nano-Claw Device Pairing Implementation
 * Copyright 2024 Nano-Claw Contributors
 */

#include "cap_pairing.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "cap_pairing";
static const char ALPHANUM[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

#define MAX_PAIRING_REQUESTS 16

typedef struct {
    pairing_request_t requests[MAX_PAIRING_REQUESTS];
    SemaphoreHandle_t mutex;
    bool initialized;
} pairing_ctx_t;

static pairing_ctx_t s_pair_ctx = {0};

static void generate_random_code(char *code) {
    for (int i = 0; i < PAIRING_CODE_LENGTH; i++) {
        code[i] = ALPHANUM[rand() % (sizeof(ALPHANUM) - 1)];
    }
    code[PAIRING_CODE_LENGTH] = '\0';
}

esp_err_t pairing_init(void) {
    if (s_pair_ctx.initialized) return ESP_OK;
    
    memset(&s_pair_ctx, 0, sizeof(pairing_ctx_t));
    s_pair_ctx.mutex = xSemaphoreCreateMutex();
    if (!s_pair_ctx.mutex) return ESP_ERR_NO_MEM;
    
    srand(esp_timer_get_time());
    s_pair_ctx.initialized = true;
    ESP_LOGI(TAG, "Pairing system initialized");
    return ESP_OK;
}

esp_err_t pairing_generate_code(pairing_channel_t channel, char *code_out) {
    if (!s_pair_ctx.initialized || !code_out) return ESP_ERR_INVALID_ARG;
    if (channel >= CHANNEL_MAX) return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(s_pair_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
    
    // Check pending limit
    uint32_t pending = 0;
    uint32_t now = esp_timer_get_time() / 1000;
    for (int i = 0; i < MAX_PAIRING_REQUESTS; i++) {
        if (s_pair_ctx.requests[i].status == PAIRING_STATUS_PENDING &&
            s_pair_ctx.requests[i].channel == channel &&
            now < s_pair_ctx.requests[i].expires_at) {
            pending++;
        }
    }
    
    if (pending >= PAIRING_MAX_PENDING) {
        xSemaphoreGive(s_pair_ctx.mutex);
        ESP_LOGW(TAG, "Max pending requests for channel %d", channel);
        return ESP_ERR_NO_MEM;
    }
    
    // Find empty slot
    int slot = -1;
    for (int i = 0; i < MAX_PAIRING_REQUESTS; i++) {
        if (s_pair_ctx.requests[i].status == PAIRING_STATUS_EXPIRED ||
            s_pair_ctx.requests[i].code[0] == '\0') {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        xSemaphoreGive(s_pair_ctx.mutex);
        return ESP_ERR_NO_MEM;
    }
    
    // Generate unique code
    pairing_request_t *req = &s_pair_ctx.requests[slot];
    bool unique;
    int attempts = 0;
    do {
        unique = true;
        generate_random_code(req->code);
        for (int i = 0; i < MAX_PAIRING_REQUESTS; i++) {
            if (i != slot && strcmp(s_pair_ctx.requests[i].code, req->code) == 0) {
                unique = false;
                break;
            }
        }
        attempts++;
    } while (!unique && attempts < 10);
    
    // Setup request
    uint32_t now = esp_timer_get_time() / 1000;
    req->channel = channel;
    req->status = PAIRING_STATUS_PENDING;
    req->created_at = now;
    req->expires_at = now + PAIRING_TTL_SEC;
    req->user_id[0] = '\0';
    
    strcpy(code_out, req->code);
    
    xSemaphoreGive(s_pair_ctx.mutex);
    ESP_LOGI(TAG, "Generated pairing code: %s (channel: %d)", code_out, channel);
    return ESP_OK;
}

esp_err_t pairing_accept(const char *code, const char *user_id) {
    if (!s_pair_ctx.initialized || !code || !user_id) return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(s_pair_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
    
    uint32_t now = esp_timer_get_time() / 1000;
    esp_err_t ret = ESP_ERR_NOT_FOUND;
    
    for (int i = 0; i < MAX_PAIRING_REQUESTS; i++) {
        if (strcmp(s_pair_ctx.requests[i].code, code) == 0) {
            if (s_pair_ctx.requests[i].status != PAIRING_STATUS_PENDING) {
                ret = ESP_ERR_INVALID_STATE;
                break;
            }
            if (now > s_pair_ctx.requests[i].expires_at) {
                s_pair_ctx.requests[i].status = PAIRING_STATUS_EXPIRED;
                ret = ESP_ERR_TIMEOUT;
                break;
            }
            
            s_pair_ctx.requests[i].status = PAIRING_STATUS_ACCEPTED;
            strncpy(s_pair_ctx.requests[i].user_id, user_id, 31);
            s_pair_ctx.requests[i].user_id[31] = '\0';
            ret = ESP_OK;
            break;
        }
    }
    
    xSemaphoreGive(s_pair_ctx.mutex);
    return ret;
}

esp_err_t pairing_revoke(const char *code) {
    if (!s_pair_ctx.initialized || !code) return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(s_pair_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
    
    esp_err_t ret = ESP_ERR_NOT_FOUND;
    for (int i = 0; i < MAX_PAIRING_REQUESTS; i++) {
        if (strcmp(s_pair_ctx.requests[i].code, code) == 0) {
            s_pair_ctx.requests[i].status = PAIRING_STATUS_REVOKED;
            ret = ESP_OK;
            break;
        }
    }
    
    xSemaphoreGive(s_pair_ctx.mutex);
    return ret;
}

esp_err_t pairing_get_request(const char *code, pairing_request_t *req) {
    if (!s_pair_ctx.initialized || !code || !req) return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(s_pair_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
    
    esp_err_t ret = ESP_ERR_NOT_FOUND;
    for (int i = 0; i < MAX_PAIRING_REQUESTS; i++) {
        if (strcmp(s_pair_ctx.requests[i].code, code) == 0) {
            memcpy(req, &s_pair_ctx.requests[i], sizeof(pairing_request_t));
            ret = ESP_OK;
            break;
        }
    }
    
    xSemaphoreGive(s_pair_ctx.mutex);
    return ret;
}

esp_err_t pairing_cleanup_expired(void) {
    if (!s_pair_ctx.initialized) return ESP_OK;
    
    if (xSemaphoreTake(s_pair_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
    
    uint32_t now = esp_timer_get_time() / 1000;
    int cleaned = 0;
    
    for (int i = 0; i < MAX_PAIRING_REQUESTS; i++) {
        if (s_pair_ctx.requests[i].status == PAIRING_STATUS_PENDING &&
            now > s_pair_ctx.requests[i].expires_at) {
            s_pair_ctx.requests[i].status = PAIRING_STATUS_EXPIRED;
            cleaned++;
        }
    }
    
    xSemaphoreGive(s_pair_ctx.mutex);
    if (cleaned > 0) ESP_LOGD(TAG, "Cleaned up %d expired requests", cleaned);
    return ESP_OK;
}

uint32_t pairing_get_pending_count(pairing_channel_t channel) {
    if (!s_pair_ctx.initialized) return 0;
    
    uint32_t count = 0;
    uint32_t now = esp_timer_get_time() / 1000;
    
    if (xSemaphoreTake(s_pair_ctx.mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        for (int i = 0; i < MAX_PAIRING_REQUESTS; i++) {
            if (s_pair_ctx.requests[i].status == PAIRING_STATUS_PENDING &&
                (channel == CHANNEL_MAX || s_pair_ctx.requests[i].channel == channel) &&
                now < s_pair_ctx.requests[i].expires_at) {
                count++;
            }
        }
        xSemaphoreGive(s_pair_ctx.mutex);
    }
    
    return count;
}
