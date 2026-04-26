#include "cap_cron.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "string.h"
#include "stdlib.h"

static const char *TAG = "cron";
static cron_job_t jobs[CRON_MAX_JOBS];
static int next_job_id = 1;
static esp_timer_handle_t cron_timer;
static SemaphoreHandle_t jobs_mutex;

static void parse_cron_expression(const char *expr, uint32_t *interval_seconds) {
    // Simple parser for common patterns
    if (strcmp(expr, "@hourly") == 0) {
        *interval_seconds = 3600;
    } else if (strcmp(expr, "@daily") == 0) {
        *interval_seconds = 86400;
    } else if (strncmp(expr, "*/", 2) == 0) {
        // */N pattern (e.g., */5 * * * *)
        int minutes = atoi(expr + 2);
        *interval_seconds = minutes * 60;
    } else {
        // Default to 1 hour for unknown expressions
        *interval_seconds = 3600;
        ESP_LOGW(TAG, "Unknown cron expression: %s, defaulting to hourly", expr);
    }
}

static void cron_task(void *arg) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        if (xSemaphoreTake(jobs_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
            continue;
        }
        
        TickType_t now = xTaskGetTickCount();
        
        for (int i = 0; i < CRON_MAX_JOBS; i++) {
            if (jobs[i].enabled && now >= jobs[i].next_run) {
                ESP_LOGD(TAG, "Running job %d", jobs[i].id);
                
                // Execute callback
                if (jobs[i].callback) {
                    jobs[i].callback(jobs[i].arg);
                }
                
                // Schedule next run
                if (jobs[i].type == CRON_TYPE_ONCE) {
                    jobs[i].enabled = false;
                } else {
                    jobs[i].next_run = now + jobs[i].interval_ticks;
                }
            }
        }
        
        xSemaphoreGive(jobs_mutex);
    }
}

esp_err_t cron_init(void) {
    memset(jobs, 0, sizeof(jobs));
    jobs_mutex = xSemaphoreCreateMutex();
    
    if (!jobs_mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Create cron task
    xTaskCreate(cron_task, "cron_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Cron scheduler initialized");
    return ESP_OK;
}

esp_err_t cron_deinit(void) {
    vSemaphoreDelete(jobs_mutex);
    memset(jobs, 0, sizeof(jobs));
    return ESP_OK;
}

cron_job_id_t cron_schedule_once(uint32_t delay_seconds, cron_callback_t callback, void *arg) {
    if (xSemaphoreTake(jobs_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return -1;
    }
    
    int id = -1;
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (!jobs[i].enabled) {
            jobs[i].id = next_job_id++;
            jobs[i].type = CRON_TYPE_ONCE;
            jobs[i].interval_ticks = pdMS_TO_TICKS(delay_seconds * 1000);
            jobs[i].callback = callback;
            jobs[i].arg = arg;
            jobs[i].next_run = xTaskGetTickCount() + jobs[i].interval_ticks;
            jobs[i].enabled = true;
            id = jobs[i].id;
            break;
        }
    }
    
    xSemaphoreGive(jobs_mutex);
    
    if (id > 0) {
        ESP_LOGI(TAG, "Scheduled one-shot job %d in %d seconds", id, delay_seconds);
    }
    
    return id;
}

cron_job_id_t cron_schedule_recurring(uint32_t interval_seconds, cron_callback_t callback, void *arg) {
    if (xSemaphoreTake(jobs_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return -1;
    }
    
    int id = -1;
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (!jobs[i].enabled) {
            jobs[i].id = next_job_id++;
            jobs[i].type = CRON_TYPE_RECURRING;
            jobs[i].interval_ticks = pdMS_TO_TICKS(interval_seconds * 1000);
            jobs[i].callback = callback;
            jobs[i].arg = arg;
            jobs[i].next_run = xTaskGetTickCount() + jobs[i].interval_ticks;
            jobs[i].enabled = true;
            id = jobs[i].id;
            break;
        }
    }
    
    xSemaphoreGive(jobs_mutex);
    
    if (id > 0) {
        ESP_LOGI(TAG, "Scheduled recurring job %d every %d seconds", id, interval_seconds);
    }
    
    return id;
}

cron_job_id_t cron_schedule(const char *cron_expr, cron_callback_t callback, void *arg) {
    uint32_t interval_seconds;
    parse_cron_expression(cron_expr, &interval_seconds);
    
    if (xSemaphoreTake(jobs_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return -1;
    }
    
    int id = -1;
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (!jobs[i].enabled) {
            jobs[i].id = next_job_id++;
            jobs[i].type = CRON_TYPE_CRON_EXPR;
            strncpy(jobs[i].expression, cron_expr, CRON_MAX_EXPR_LEN - 1);
            jobs[i].interval_ticks = pdMS_TO_TICKS(interval_seconds * 1000);
            jobs[i].callback = callback;
            jobs[i].arg = arg;
            jobs[i].next_run = xTaskGetTickCount() + jobs[i].interval_ticks;
            jobs[i].enabled = true;
            id = jobs[i].id;
            break;
        }
    }
    
    xSemaphoreGive(jobs_mutex);
    
    if (id > 0) {
        ESP_LOGI(TAG, "Scheduled cron job %d with expression: %s", id, cron_expr);
    }
    
    return id;
}

esp_err_t cron_cancel(cron_job_id_t job_id) {
    if (xSemaphoreTake(jobs_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (jobs[i].id == job_id) {
            jobs[i].enabled = false;
            memset(&jobs[i], 0, sizeof(cron_job_t));
            xSemaphoreGive(jobs_mutex);
            ESP_LOGI(TAG, "Cancelled job %d", job_id);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(jobs_mutex);
    ESP_LOGW(TAG, "Job %d not found", job_id);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t cron_enable(cron_job_id_t job_id) {
    if (xSemaphoreTake(jobs_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (jobs[i].id == job_id) {
            jobs[i].enabled = true;
            jobs[i].next_run = xTaskGetTickCount();
            xSemaphoreGive(jobs_mutex);
            ESP_LOGI(TAG, "Enabled job %d", job_id);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(jobs_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t cron_disable(cron_job_id_t job_id) {
    if (xSemaphoreTake(jobs_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (jobs[i].id == job_id) {
            jobs[i].enabled = false;
            xSemaphoreGive(jobs_mutex);
            ESP_LOGI(TAG, "Disabled job %d", job_id);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(jobs_mutex);
    return ESP_ERR_NOT_FOUND;
}

int cron_get_active_jobs(void) {
    if (xSemaphoreTake(jobs_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return -1;
    }
    
    int count = 0;
    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        if (jobs[i].enabled) {
            count++;
        }
    }
    
    xSemaphoreGive(jobs_mutex);
    return count;
}
