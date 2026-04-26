/*
 * Nano-Claw Heartbeat Monitor Implementation
 * Copyright 2024 Nano-Claw Contributors
 */

#include "cap_heartbeat.h"
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "cap_heartbeat";

typedef struct {
    heartbeat_config_t config;
    TaskHandle_t task_handle;
    SemaphoreHandle_t mutex;
    heartbeat_status_t status;
    uint32_t start_time;
    uint32_t beat_count;
    uint32_t failure_count;
    uint32_t reconnect_count;
    bool initialized;
} heartbeat_ctx_t;

static heartbeat_ctx_t s_hb_ctx = {0};

static void heartbeat_task(void *pvParameters) {
    ESP_LOGI(TAG, "Heartbeat task started (interval: %lus)", s_hb_ctx.config.interval_sec);
    
    while (s_hb_ctx.status == HEARTBEAT_STATUS_RUNNING || 
           s_hb_ctx.status == HEARTBEAT_STATUS_CONNECTED) {
        
        vTaskDelay(pdMS_TO_TICKS(s_hb_ctx.config.interval_sec * 1000));
        
        if (s_hb_ctx.config.callback) {
            ESP_LOGD(TAG, "Sending heartbeat #%lu", (long)s_hb_ctx.beat_count);
            s_hb_ctx.config.callback(true, s_hb_ctx.config.user_data);
            s_hb_ctx.beat_count++;
            s_hb_ctx.status = HEARTBEAT_STATUS_CONNECTED;
        }
    }
    
    ESP_LOGI(TAG, "Heartbeat task stopped");
    vTaskDelete(NULL);
}

esp_err_t heartbeat_init(void) {
    if (s_hb_ctx.initialized) return ESP_OK;
    
    memset(&s_hb_ctx, 0, sizeof(heartbeat_ctx_t));
    s_hb_ctx.mutex = xSemaphoreCreateMutex();
    if (!s_hb_ctx.mutex) return ESP_ERR_NO_MEM;
    
    s_hb_ctx.status = HEARTBEAT_STATUS_STOPPED;
    s_hb_ctx.initialized = true;
    ESP_LOGI(TAG, "Heartbeat monitor initialized");
    return ESP_OK;
}

esp_err_t heartbeat_start(heartbeat_config_t *config) {
    if (!s_hb_ctx.initialized || !config) return ESP_ERR_INVALID_ARG;
    if (config->interval_sec < HEARTBEAT_MIN_INTERVAL_SEC || 
        config->interval_sec > HEARTBEAT_MAX_INTERVAL_SEC) 
        return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(s_hb_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
    
    if (s_hb_ctx.status == HEARTBEAT_STATUS_RUNNING) {
        xSemaphoreGive(s_hb_ctx.mutex);
        return ESP_ERR_INVALID_STATE;
    }
    
    memcpy(&s_hb_ctx.config, config, sizeof(heartbeat_config_t));
    s_hb_ctx.status = HEARTBEAT_STATUS_RUNNING;
    s_hb_ctx.start_time = esp_timer_get_time() / 1000;
    s_hb_ctx.beat_count = 0;
    s_hb_ctx.failure_count = 0;
    s_hb_ctx.reconnect_count = 0;
    
    xTaskCreate(heartbeat_task, "heartbeat_task", 3072, NULL, 5, 
                &s_hb_ctx.task_handle);
    
    xSemaphoreGive(s_hb_ctx.mutex);
    return ESP_OK;
}

esp_err_t heartbeat_stop(void) {
    if (!s_hb_ctx.initialized) return ESP_OK;
    
    s_hb_ctx.status = HEARTBEAT_STATUS_STOPPED;
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}

esp_err_t heartbeat_set_interval(uint32_t interval_sec) {
    if (!s_hb_ctx.initialized) return ESP_ERR_INVALID_STATE;
    if (interval_sec < HEARTBEAT_MIN_INTERVAL_SEC || 
        interval_sec > HEARTBEAT_MAX_INTERVAL_SEC)
        return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(s_hb_ctx.mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        s_hb_ctx.config.interval_sec = interval_sec;
        xSemaphoreGive(s_hb_ctx.mutex);
        return ESP_OK;
    }
    return ESP_ERR_TIMEOUT;
}

heartbeat_status_t heartbeat_get_status(void) {
    return s_hb_ctx.status;
}

uint32_t heartbeat_get_uptime(void) {
    if (s_hb_ctx.status == HEARTBEAT_STATUS_STOPPED) return 0;
    return (esp_timer_get_time() / 1000) - s_hb_ctx.start_time;
}

uint32_t heartbeat_get_beat_count(void) {
    return s_hb_ctx.beat_count;
}

void heartbeat_get_stats(uint32_t *total_beats, uint32_t *failures, 
                         uint32_t *reconnects) {
    if (total_beats) *total_beats = s_hb_ctx.beat_count;
    if (failures) *failures = s_hb_ctx.failure_count;
    if (reconnects) *reconnects = s_hb_ctx.reconnect_count;
}
