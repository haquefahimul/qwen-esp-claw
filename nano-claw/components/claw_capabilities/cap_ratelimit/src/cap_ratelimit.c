/* Nano-Claw Rate Limiter Implementation - Copyright 2024 */
#include "cap_ratelimit.h"
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/semphr.h"

static const char *TAG = "cap_ratelimit";
#define MAX_LIMITERS 16
#define WINDOW_SIZE 60

typedef struct {
    char name[32];
    ratelimit_config_t config;
    uint32_t timestamps[WINDOW_SIZE];
    uint32_t count;
    uint32_t head;
} limiter_t;

static struct {
    limiter_t limiters[MAX_LIMITERS];
    SemaphoreHandle_t mutex;
    bool initialized;
} s_rl_ctx;

esp_err_t ratelimit_init(void) {
    if (s_rl_ctx.initialized) return ESP_OK;
    memset(&s_rl_ctx, 0, sizeof(s_rl_ctx));
    s_rl_ctx.mutex = xSemaphoreCreateMutex();
    s_rl_ctx.initialized = true;
    ESP_LOGI(TAG, "Rate limiter initialized");
    return ESP_OK;
}

esp_err_t ratelimit_create(const char *name, ratelimit_config_t *cfg) {
    if (!s_rl_ctx.initialized || !name || !cfg) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(s_rl_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) return ESP_ERR_TIMEOUT;
    
    for (int i = 0; i < MAX_LIMITERS; i++) {
        if (s_rl_ctx.limiters[i].name[0] == '\0') {
            strncpy(s_rl_ctx.limiters[i].name, name, 31);
            memcpy(&s_rl_ctx.limiters[i].config, cfg, sizeof(ratelimit_config_t));
            memset(s_rl_ctx.limiters[i].timestamps, 0, sizeof(s_rl_ctx.limiters[i].timestamps));
            s_rl_ctx.limiters[i].count = 0;
            s_rl_ctx.limiters[i].head = 0;
            xSemaphoreGive(s_rl_ctx.mutex);
            ESP_LOGI(TAG, "Rate limiter '%s' created", name);
            return ESP_OK;
        }
    }
    xSemaphoreGive(s_rl_ctx.mutex);
    return ESP_ERR_NO_MEM;
}

esp_err_t ratelimit_check(const char *name, bool *allowed) {
    if (!s_rl_ctx.initialized || !name || !allowed) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(s_rl_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) return ESP_ERR_TIMEOUT;
    
    int slot = -1;
    for (int i = 0; i < MAX_LIMITERS; i++) {
        if (strcmp(s_rl_ctx.limiters[i].name, name) == 0) { slot = i; break; }
    }
    
    if (slot == -1) { xSemaphoreGive(s_rl_ctx.mutex); *allowed = true; return ESP_OK; }
    
    limiter_t *lim = &s_rl_ctx.limiters[slot];
    uint32_t now = esp_timer_get_time() / 1000;
    uint32_t window_start = now - lim->config.window_ms;
    
    // Remove old timestamps
    while (lim->count > 0 && lim->timestamps[lim->head] < window_start) {
        lim->head = (lim->head + 1) % WINDOW_SIZE;
        lim->count--;
    }
    
    *allowed = (lim->count < lim->config.max_requests + lim->config.burst);
    if (*allowed) {
        uint32_t tail = (lim->head + lim->count) % WINDOW_SIZE;
        lim->timestamps[tail] = now;
        lim->count++;
    }
    
    xSemaphoreGive(s_rl_ctx.mutex);
    return ESP_OK;
}

esp_err_t ratelimit_reset(const char *name) {
    if (!s_rl_ctx.initialized || !name) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(s_rl_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) return ESP_ERR_TIMEOUT;
    for (int i = 0; i < MAX_LIMITERS; i++) {
        if (strcmp(s_rl_ctx.limiters[i].name, name) == 0) {
            s_rl_ctx.limiters[i].count = 0;
            memset(s_rl_ctx.limiters[i].timestamps, 0, sizeof(s_rl_ctx.limiters[i].timestamps));
            xSemaphoreGive(s_rl_ctx.mutex);
            return ESP_OK;
        }
    }
    xSemaphoreGive(s_rl_ctx.mutex);
    return ESP_ERR_NOT_FOUND;
}

void ratelimit_cleanup(void) {
    if (s_rl_ctx.mutex) vSemaphoreDelete(s_rl_ctx.mutex);
    memset(&s_rl_ctx, 0, sizeof(s_rl_ctx));
}
