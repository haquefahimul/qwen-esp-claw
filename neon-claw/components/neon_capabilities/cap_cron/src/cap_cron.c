/**
 * @file cap_cron.c
 * @brief Cron Scheduler Implementation
 */

#include "cap_cron.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <stdlib.h>

static const char* TAG = "cron";
static cron_job_t jobs[CRON_MAX_JOBS];
static int job_count = 0;
static bool initialized = false;
static esp_timer_handle_t timer_handle;

// Forward declarations
static void cron_task(void* arg);
static uint32_t parse_cron_expression(const char* expr, uint32_t now);

esp_err_t cron_init(void) {
    if (initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    memset(jobs, 0, sizeof(jobs));
    job_count = 0;
    
    // Create timer for periodic checking (every 1 second)
    esp_timer_create_args_t timer_args = {
        .callback = cron_task,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "cron_timer",
        .skip_unhandled_events = false
    };
    
    esp_err_t ret = esp_timer_create(&timer_args, &timer_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer");
        return ret;
    }
    
    ret = esp_timer_start_periodic(timer_handle, 1000000); // 1 second
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start timer");
        esp_timer_delete(timer_handle);
        return ret;
    }
    
    initialized = true;
    ESP_LOGI(TAG, "Cron scheduler initialized");
    return ESP_OK;
}

esp_err_t cron_deinit(void) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_timer_stop(timer_handle);
    esp_timer_delete(timer_handle);
    
    // Remove all jobs
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (jobs[i].active) {
            jobs[i].active = false;
        }
    }
    
    job_count = 0;
    initialized = false;
    ESP_LOGI(TAG, "Cron scheduler deinitialized");
    return ESP_OK;
}

esp_err_t cron_add_once(const char* id, uint32_t delay_ms, cron_callback_t cb, void* arg) {
    if (!initialized || !id || !cb) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Find empty slot
    int slot = -1;
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (!jobs[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        ESP_LOGW(TAG, "No free slots for cron job");
        return ESP_ERR_NO_MEM;
    }
    
    strncpy(jobs[slot].id, id, sizeof(jobs[slot].id) - 1);
    jobs[slot].type = CRON_ONCE;
    jobs[slot].interval_ms = delay_ms;
    jobs[slot].callback = cb;
    jobs[slot].arg = arg;
    jobs[slot].last_run = 0;
    jobs[slot].next_run = esp_timer_get_time() / 1000 + delay_ms;
    jobs[slot].active = true;
    jobs[slot].running = false;
    
    job_count++;
    ESP_LOGI(TAG, "Added one-shot job: %s (delay=%dms)", id, delay_ms);
    return ESP_OK;
}

esp_err_t cron_add_interval(const char* id, uint32_t interval_ms, cron_callback_t cb, void* arg) {
    if (!initialized || !id || !cb) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int slot = -1;
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (!jobs[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return ESP_ERR_NO_MEM;
    }
    
    strncpy(jobs[slot].id, id, sizeof(jobs[slot].id) - 1);
    jobs[slot].type = CRON_INTERVAL;
    jobs[slot].interval_ms = interval_ms;
    jobs[slot].callback = cb;
    jobs[slot].arg = arg;
    jobs[slot].last_run = 0;
    jobs[slot].next_run = esp_timer_get_time() / 1000 + interval_ms;
    jobs[slot].active = true;
    jobs[slot].running = false;
    
    job_count++;
    ESP_LOGI(TAG, "Added interval job: %s (interval=%dms)", id, interval_ms);
    return ESP_OK;
}

esp_err_t cron_add_expression(const char* id, const char* expr, cron_callback_t cb, void* arg) {
    if (!initialized || !id || !expr || !cb) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int slot = -1;
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (!jobs[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return ESP_ERR_NO_MEM;
    }
    
    strncpy(jobs[slot].id, id, sizeof(jobs[slot].id) - 1);
    jobs[slot].type = CRON_EXPRESSION;
    strncpy(jobs[slot].expression, expr, sizeof(jobs[slot].expression) - 1);
    jobs[slot].callback = cb;
    jobs[slot].arg = arg;
    jobs[slot].last_run = 0;
    
    uint32_t now = esp_timer_get_time() / 1000;
    jobs[slot].next_run = parse_cron_expression(expr, now);
    jobs[slot].active = true;
    jobs[slot].running = false;
    
    job_count++;
    ESP_LOGI(TAG, "Added cron job: %s (expr=%s, next=%ds)", id, expr, jobs[slot].next_run);
    return ESP_OK;
}

esp_err_t cron_remove(const char* id) {
    if (!initialized || !id) {
        return ESP_ERR_INVALID_ARG;
    }
    
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (jobs[i].active && strcmp(jobs[i].id, id) == 0) {
            jobs[i].active = false;
            job_count--;
            ESP_LOGI(TAG, "Removed job: %s", id);
            return ESP_OK;
        }
    }
    
    return ESP_ERR_NOT_FOUND;
}

esp_err_t cron_pause(const char* id) {
    if (!initialized || !id) {
        return ESP_ERR_INVALID_ARG;
    }
    
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (jobs[i].active && strcmp(jobs[i].id, id) == 0) {
            jobs[i].active = false;
            ESP_LOGI(TAG, "Paused job: %s", id);
            return ESP_OK;
        }
    }
    
    return ESP_ERR_NOT_FOUND;
}

esp_err_t cron_resume(const char* id) {
    if (!initialized || !id) {
        return ESP_ERR_INVALID_ARG;
    }
    
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (!jobs[i].active && strcmp(jobs[i].id, id) == 0) {
            jobs[i].active = true;
            ESP_LOGI(TAG, "Resumed job: %s", id);
            return ESP_OK;
        }
    }
    
    return ESP_ERR_NOT_FOUND;
}

bool cron_is_active(const char* id) {
    if (!initialized || !id) {
        return false;
    }
    
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (jobs[i].active && strcmp(jobs[i].id, id) == 0) {
            return true;
        }
    }
    
    return false;
}

uint32_t cron_get_next_run(const char* id) {
    if (!initialized || !id) {
        return 0;
    }
    
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (jobs[i].active && strcmp(jobs[i].id, id) == 0) {
            return jobs[i].next_run;
        }
    }
    
    return 0;
}

esp_err_t cron_trigger_now(const char* id) {
    if (!initialized || !id) {
        return ESP_ERR_INVALID_ARG;
    }
    
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (jobs[i].active && strcmp(jobs[i].id, id) == 0) {
            if (jobs[i].running) {
                ESP_LOGW(TAG, "Job %s is already running", id);
                return ESP_ERR_INVALID_STATE;
            }
            
            jobs[i].callback(jobs[i].arg);
            jobs[i].last_run = esp_timer_get_time() / 1000;
            ESP_LOGI(TAG, "Triggered job: %s", id);
            return ESP_OK;
        }
    }
    
    return ESP_ERR_NOT_FOUND;
}

int cron_get_job_count(void) {
    return job_count;
}

// Simple cron expression parser (supports basic formats)
static uint32_t parse_cron_expression(const char* expr, uint32_t now) {
    // Handle shortcuts
    if (strcmp(expr, "@hourly") == 0) {
        return now + 3600000; // 1 hour in ms
    }
    if (strcmp(expr, "@daily") == 0) {
        return now + 86400000; // 24 hours in ms
    }
    if (strcmp(expr, "@reboot") == 0) {
        return now + 5000; // 5 seconds after boot
    }
    
    // Parse standard format: "*/N * * * *" or "N * * * *"
    int minute = -1, interval = -1;
    if (sscanf(expr, "*/%d", &interval) == 1) {
        // Round up to next interval
        int mins = (now / 60000) % 60;
        int next_min = ((mins / interval) + 1) * interval;
        if (next_min >= 60) next_min = 0;
        return ((now / 3600000) * 3600000) + (next_min * 60000);
    }
    if (sscanf(expr, "%d", &minute) == 1) {
        int current_min = (now / 60000) % 60;
        if (minute <= current_min) {
            return now + (60 - current_min + minute) * 60000;
        }
        return now + (minute - current_min) * 60000;
    }
    
    // Default: run every minute
    return now + 60000;
}

static void cron_task(void* arg) {
    uint32_t now = esp_timer_get_time() / 1000;
    
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (!jobs[i].active || jobs[i].running) continue;
        
        if (now >= jobs[i].next_run) {
            jobs[i].running = true;
            jobs[i].callback(jobs[i].arg);
            jobs[i].last_run = now;
            
            // Calculate next run time
            switch (jobs[i].type) {
                case CRON_ONCE:
                    jobs[i].active = false;
                    job_count--;
                    break;
                case CRON_INTERVAL:
                    jobs[i].next_run = now + jobs[i].interval_ms;
                    break;
                case CRON_EXPRESSION:
                    jobs[i].next_run = parse_cron_expression(jobs[i].expression, now);
                    break;
            }
            
            jobs[i].running = false;
        }
    }
}
