/*
 * Nano-Claw Cron Scheduler Implementation
 * 
 * Copyright 2024 Nano-Claw Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cap_cron.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "cap_cron";

/**
 * @brief Cron scheduler context
 */
typedef struct {
    cron_job_t jobs[CRON_MAX_JOBS];
    SemaphoreHandle_t mutex;
    TaskHandle_t task_handle;
    bool initialized;
} cron_context_t;

static cron_context_t s_cron_ctx = {0};

/**
 * @brief Parse cron expression field
 */
static bool parse_cron_field(const char *field, int min_val, int max_val, 
                             int *result) {
    if (strcmp(field, "*") == 0) {
        *result = -1; // All values
        return true;
    }
    
    if (strncmp(field, "*/", 2) == 0) {
        int step = atoi(field + 2);
        if (step > 0 && step <= max_val) {
            *result = step;
            return true;
        }
        return false;
    }
    
    int val = atoi(field);
    if (val >= min_val && val <= max_val) {
        *result = val;
        return true;
    }
    
    return false;
}

/**
 * @brief Calculate next run time for cron expression
 */
static uint32_t calculate_next_run(const char *expression, uint32_t current_time) {
    // Simplified cron parser - supports basic expressions
    // Format: minute hour day month weekday
    
    if (expression[0] == '@') {
        // Special expressions
        if (strcmp(expression, "@hourly") == 0) {
            return current_time + 3600000; // 1 hour in ms
        } else if (strcmp(expression, "@daily") == 0 || 
                   strcmp(expression, "@midnight") == 0) {
            return current_time + 86400000; // 24 hours in ms
        } else if (strcmp(expression, "@weekly") == 0) {
            return current_time + 604800000; // 7 days in ms
        }
    }
    
    // Parse standard cron expression (simplified)
    char expr_copy[CRON_EXPRESSION_MAX];
    strncpy(expr_copy, expression, CRON_EXPRESSION_MAX - 1);
    expr_copy[CRON_EXPRESSION_MAX - 1] = '\0';
    
    char *minute = strtok(expr_copy, " \t");
    char *hour = strtok(NULL, " \t");
    
    if (!minute || !hour) {
        return 0; // Invalid expression
    }
    
    int min_val = 0, hour_val = 0;
    
    if (!parse_cron_field(minute, 0, 59, &min_val)) {
        min_val = 0;
    }
    if (!parse_cron_field(hour, 0, 23, &hour_val)) {
        hour_val = 0;
    }
    
    // Calculate next occurrence (simplified)
    uint32_t now_sec = current_time / 1000;
    uint32_t current_min = (now_sec / 60) % 60;
    uint32_t current_hour = (now_sec / 3600) % 24;
    
    uint32_t wait_minutes = 0;
    
    if (hour_val == -1) {
        // Every hour
        if (min_val == -1) {
            wait_minutes = 1; // Every minute
        } else {
            wait_minutes = (min_val > current_min) ? 
                          (min_val - current_min) : 
                          (60 - current_min + min_val);
        }
    } else {
        // Specific hour
        if (hour_val > current_hour) {
            wait_minutes = (hour_val - current_hour) * 60 + min_val;
        } else if (hour_val == current_hour && min_val > current_min) {
            wait_minutes = min_val - current_min;
        } else {
            wait_minutes = (24 - current_hour + hour_val) * 60 + min_val;
        }
    }
    
    return current_time + (wait_minutes * 60000);
}

/**
 * @brief Check if job should run
 */
static bool should_run_job(cron_job_t *job, uint32_t current_time) {
    if (!job->enabled || job->status == CRON_STATUS_RUNNING) {
        return false;
    }
    
    if (job->max_runs > 0 && job->run_count >= job->max_runs) {
        return false;
    }
    
    switch (job->type) {
        case CRON_TYPE_ONESHOT:
            return current_time >= job->next_run_time;
            
        case CRON_TYPE_RECURRING:
            return current_time >= job->next_run_time;
            
        case CRON_TYPE_CRON_EXPR:
            return current_time >= job->next_run_time;
            
        default:
            return false;
    }
}

/**
 * @brief Execute a cron job
 */
static void execute_job(cron_job_t *job) {
    job->status = CRON_STATUS_RUNNING;
    job->last_run_time = esp_timer_get_time() / 1000;
    job->run_count++;
    
    ESP_LOGD(TAG, "Executing job: %s (run #%lu)", job->name, (long)job->run_count);
    
    // Execute callback
    if (job->callback) {
        job->callback(job->user_data);
    }
    
    job->status = CRON_STATUS_COMPLETED;
    
    // Update next run time
    uint32_t current_time = esp_timer_get_time() / 1000;
    
    switch (job->type) {
        case CRON_TYPE_ONESHOT:
            job->enabled = false;
            job->status = CRON_STATUS_COMPLETED;
            break;
            
        case CRON_TYPE_RECURRING:
            job->next_run_time = current_time + job->interval_ms;
            job->status = CRON_STATUS_PENDING;
            break;
            
        case CRON_TYPE_CRON_EXPR:
            job->next_run_time = calculate_next_run(job->expression, current_time);
            job->status = CRON_STATUS_PENDING;
            break;
            
        default:
            break;
    }
}

/**
 * @brief Cron scheduler task
 */
static void cron_task(void *pvParameters) {
    ESP_LOGI(TAG, "Cron scheduler started");
    
    while (s_cron_ctx.initialized) {
        uint32_t current_time = esp_timer_get_time() / 1000;
        uint32_t min_wait = UINT32_MAX;
        
        // Check all jobs
        if (xSemaphoreTake(s_cron_ctx.mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            for (int i = 0; i < CRON_MAX_JOBS; i++) {
                cron_job_t *job = &s_cron_ctx.jobs[i];
                
                if (job->name[0] != '\0' && job->enabled) {
                    if (should_run_job(job, current_time)) {
                        execute_job(job);
                    }
                    
                    // Calculate time until next run
                    if (job->enabled && job->next_run_time > current_time) {
                        uint32_t wait_time = job->next_run_time - current_time;
                        if (wait_time < min_wait) {
                            min_wait = wait_time;
                        }
                    }
                }
            }
            xSemaphoreGive(s_cron_ctx.mutex);
        }
        
        // Wait until next check (max 1 second)
        uint32_t wait_time = (min_wait == UINT32_MAX || min_wait > 1000) ? 
                            1000 : min_wait;
        vTaskDelay(pdMS_TO_TICKS(wait_time));
    }
    
    ESP_LOGI(TAG, "Cron scheduler stopped");
    vTaskDelete(NULL);
}

esp_err_t cron_init(void) {
    if (s_cron_ctx.initialized) {
        ESP_LOGW(TAG, "Cron scheduler already initialized");
        return ESP_OK;
    }
    
    memset(&s_cron_ctx, 0, sizeof(cron_context_t));
    
    s_cron_ctx.mutex = xSemaphoreCreateMutex();
    if (!s_cron_ctx.mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Create scheduler task
    BaseType_t ret = xTaskCreate(cron_task, "cron_task", 4096, NULL, 5, 
                                 &s_cron_ctx.task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create cron task");
        vSemaphoreDelete(s_cron_ctx.mutex);
        return ESP_ERR_NO_MEM;
    }
    
    s_cron_ctx.initialized = true;
    ESP_LOGI(TAG, "Cron scheduler initialized (max jobs: %d)", CRON_MAX_JOBS);
    
    return ESP_OK;
}

esp_err_t cron_deinit(void) {
    if (!s_cron_ctx.initialized) {
        return ESP_OK;
    }
    
    s_cron_ctx.initialized = false;
    
    // Wait for task to stop
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    cron_cancel_all();
    
    if (s_cron_ctx.mutex) {
        vSemaphoreDelete(s_cron_ctx.mutex);
        s_cron_ctx.mutex = NULL;
    }
    
    ESP_LOGI(TAG, "Cron scheduler deinitialized");
    return ESP_OK;
}

esp_err_t cron_schedule_once(const char *name, cron_callback_t callback,
                             void *user_data, uint32_t delay_ms) {
    if (!s_cron_ctx.initialized || !name || !callback) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_cron_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Find empty slot
    int slot = -1;
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (s_cron_ctx.jobs[i].name[0] == '\0') {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        xSemaphoreGive(s_cron_ctx.mutex);
        ESP_LOGW(TAG, "Max jobs reached (%d)", CRON_MAX_JOBS);
        return ESP_ERR_NO_MEM;
    }
    
    // Setup job
    cron_job_t *job = &s_cron_ctx.jobs[slot];
    strncpy(job->name, name, CRON_JOB_NAME_MAX - 1);
    job->name[CRON_JOB_NAME_MAX - 1] = '\0';
    job->type = CRON_TYPE_ONESHOT;
    job->callback = callback;
    job->user_data = user_data;
    job->delay_ms = delay_ms;
    job->status = CRON_STATUS_PENDING;
    job->enabled = true;
    job->run_count = 0;
    job->max_runs = 1;
    job->next_run_time = (esp_timer_get_time() / 1000) + delay_ms;
    
    xSemaphoreGive(s_cron_ctx.mutex);
    
    ESP_LOGI(TAG, "Scheduled one-shot job: %s (delay: %lu ms)", 
             name, (long)delay_ms);
    return ESP_OK;
}

esp_err_t cron_schedule_recurring(const char *name, cron_callback_t callback,
                                  void *user_data, uint32_t interval_ms,
                                  uint32_t max_runs) {
    if (!s_cron_ctx.initialized || !name || !callback || interval_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_cron_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Find empty slot
    int slot = -1;
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (s_cron_ctx.jobs[i].name[0] == '\0') {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        xSemaphoreGive(s_cron_ctx.mutex);
        return ESP_ERR_NO_MEM;
    }
    
    // Setup job
    cron_job_t *job = &s_cron_ctx.jobs[slot];
    strncpy(job->name, name, CRON_JOB_NAME_MAX - 1);
    job->name[CRON_JOB_NAME_MAX - 1] = '\0';
    job->type = CRON_TYPE_RECURRING;
    job->callback = callback;
    job->user_data = user_data;
    job->interval_ms = interval_ms;
    job->status = CRON_STATUS_PENDING;
    job->enabled = true;
    job->run_count = 0;
    job->max_runs = max_runs;
    job->next_run_time = (esp_timer_get_time() / 1000) + interval_ms;
    
    xSemaphoreGive(s_cron_ctx.mutex);
    
    ESP_LOGI(TAG, "Scheduled recurring job: %s (interval: %lu ms, max_runs: %lu)",
             name, (long)interval_ms, (long)max_runs);
    return ESP_OK;
}

esp_err_t cron_schedule_expression(const char *name, const char *expression,
                                   cron_callback_t callback, void *user_data,
                                   uint32_t max_runs) {
    if (!s_cron_ctx.initialized || !name || !expression || !callback) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_cron_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Find empty slot
    int slot = -1;
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (s_cron_ctx.jobs[i].name[0] == '\0') {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        xSemaphoreGive(s_cron_ctx.mutex);
        return ESP_ERR_NO_MEM;
    }
    
    // Setup job
    cron_job_t *job = &s_cron_ctx.jobs[slot];
    strncpy(job->name, name, CRON_JOB_NAME_MAX - 1);
    job->name[CRON_JOB_NAME_MAX - 1] = '\0';
    strncpy(job->expression, expression, CRON_EXPRESSION_MAX - 1);
    job->expression[CRON_EXPRESSION_MAX - 1] = '\0';
    job->type = CRON_TYPE_CRON_EXPR;
    job->callback = callback;
    job->user_data = user_data;
    job->status = CRON_STATUS_PENDING;
    job->enabled = true;
    job->run_count = 0;
    job->max_runs = max_runs;
    job->next_run_time = calculate_next_run(expression, esp_timer_get_time() / 1000);
    
    xSemaphoreGive(s_cron_ctx.mutex);
    
    ESP_LOGI(TAG, "Scheduled cron job: %s (expr: %s)", name, expression);
    return ESP_OK;
}

esp_err_t cron_cancel(const char *name) {
    if (!s_cron_ctx.initialized || !name) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_cron_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t ret = ESP_ERR_NOT_FOUND;
    
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (strcmp(s_cron_ctx.jobs[i].name, name) == 0) {
            s_cron_ctx.jobs[i].enabled = false;
            s_cron_ctx.jobs[i].status = CRON_STATUS_CANCELLED;
            s_cron_ctx.jobs[i].name[0] = '\0'; // Clear slot
            ret = ESP_OK;
            break;
        }
    }
    
    xSemaphoreGive(s_cron_ctx.mutex);
    return ret;
}

esp_err_t cron_cancel_all(void) {
    if (!s_cron_ctx.initialized) {
        return ESP_OK;
    }
    
    if (xSemaphoreTake(s_cron_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        s_cron_ctx.jobs[i].name[0] = '\0';
        s_cron_ctx.jobs[i].enabled = false;
        s_cron_ctx.jobs[i].status = CRON_STATUS_CANCELLED;
    }
    
    xSemaphoreGive(s_cron_ctx.mutex);
    ESP_LOGI(TAG, "All jobs cancelled");
    return ESP_OK;
}

esp_err_t cron_get_job(const char *name, cron_job_t *job) {
    if (!s_cron_ctx.initialized || !name) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_cron_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t ret = ESP_ERR_NOT_FOUND;
    
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (strcmp(s_cron_ctx.jobs[i].name, name) == 0) {
            if (job) {
                memcpy(job, &s_cron_ctx.jobs[i], sizeof(cron_job_t));
            }
            ret = ESP_OK;
            break;
        }
    }
    
    xSemaphoreGive(s_cron_ctx.mutex);
    return ret;
}

esp_err_t cron_set_enabled(const char *name, bool enabled) {
    if (!s_cron_ctx.initialized || !name) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_cron_ctx.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t ret = ESP_ERR_NOT_FOUND;
    
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (strcmp(s_cron_ctx.jobs[i].name, name) == 0) {
            s_cron_ctx.jobs[i].enabled = enabled;
            ret = ESP_OK;
            break;
        }
    }
    
    xSemaphoreGive(s_cron_ctx.mutex);
    return ret;
}

uint32_t cron_get_job_count(void) {
    if (!s_cron_ctx.initialized) {
        return 0;
    }
    
    uint32_t count = 0;
    
    if (xSemaphoreTake(s_cron_ctx.mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        for (int i = 0; i < CRON_MAX_JOBS; i++) {
            if (s_cron_ctx.jobs[i].name[0] != '\0') {
                count++;
            }
        }
        xSemaphoreGive(s_cron_ctx.mutex);
    }
    
    return count;
}

void cron_get_stats(uint32_t *total_jobs, uint32_t *running_jobs,
                    uint32_t *pending_jobs) {
    uint32_t total = 0, running = 0, pending = 0;
    
    if (s_cron_ctx.initialized && 
        xSemaphoreTake(s_cron_ctx.mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        
        for (int i = 0; i < CRON_MAX_JOBS; i++) {
            if (s_cron_ctx.jobs[i].name[0] != '\0') {
                total++;
                switch (s_cron_ctx.jobs[i].status) {
                    case CRON_STATUS_RUNNING:
                        running++;
                        break;
                    case CRON_STATUS_PENDING:
                        pending++;
                        break;
                    default:
                        break;
                }
            }
        }
        xSemaphoreGive(s_cron_ctx.mutex);
    }
    
    if (total_jobs) *total_jobs = total;
    if (running_jobs) *running_jobs = running;
    if (pending_jobs) *pending_jobs = pending;
}
