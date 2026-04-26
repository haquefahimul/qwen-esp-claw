/**
 * @file cap_health.c
 * @brief Implementation of Task Watchdog & System Health Monitor
 */

#include "cap_health.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char* TAG = "cap_health";

static TaskHandle_t s_monitor_task = NULL;
static bool s_initialized = false;
static uint32_t s_watchdog_timeout_ms = 0;

// Thresholds
#define HEAP_CRITICAL_THRESHOLD (1024 * 50)     // 50KB
#define STACK_CRITICAL_THRESHOLD 512            // 512 bytes
#define PSRAM_CRITICAL_THRESHOLD (1024 * 500)   // 500KB

esp_err_t health_init(uint32_t watchdog_timeout_ms) {
    if (s_initialized) {
        return ESP_OK;
    }

    s_watchdog_timeout_ms = watchdog_timeout_ms;

    if (watchdog_timeout_ms > 0) {
        esp_task_wdt_config_t wdt_config = {
            .timeout_ms = watchdog_timeout_ms,
            .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
            .trigger_panic = true,
        };
        esp_task_wdt_init(&wdt_config);
        ESP_LOGI(TAG, "Watchdog initialized with %dms timeout", watchdog_timeout_ms);
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Health subsystem initialized");
    return ESP_OK;
}

static void health_monitor_task(void* pvParameters) {
    uint32_t interval_ms = *(uint32_t*)pvParameters;
    free(pvParameters);

    while (1) {
        health_status_t status;
        health_get_status(&status);

        if (!status.is_healthy) {
            ESP_LOGE(TAG, "System unhealthy! Free heap: %d, Min heap: %d", 
                     status.free_heap, status.min_free_heap);
            
            if (status.free_heap < HEAP_CRITICAL_THRESHOLD) {
                ESP_LOGE(TAG, "Critical: Heap too low, rebooting...");
                vTaskDelay(pdMS_TO_TICKS(1000));
                health_reboot_system();
            }
        }

        // Feed watchdog
        if (s_watchdog_timeout_ms > 0) {
            esp_task_wdt_reset();
        }

        vTaskDelay(pdMS_TO_TICKS(interval_ms));
    }
}

esp_err_t health_start_monitor(uint32_t check_interval_ms) {
    if (s_monitor_task) {
        ESP_LOGW(TAG, "Monitor already running");
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t* interval = malloc(sizeof(uint32_t));
    if (!interval) return ESP_ERR_NO_MEM;
    *interval = check_interval_ms;

    BaseType_t ret = xTaskCreate(
        health_monitor_task,
        "health_mon",
        2048,
        interval,
        5,
        &s_monitor_task
    );

    if (ret != pdPASS) {
        free(interval);
        ESP_LOGE(TAG, "Failed to create monitor task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Health monitor started (%dms interval)", check_interval_ms);
    return ESP_OK;
}

void health_stop_monitor(void) {
    if (s_monitor_task) {
        vTaskDelete(s_monitor_task);
        s_monitor_task = NULL;
        ESP_LOGI(TAG, "Health monitor stopped");
    }
}

esp_err_t health_get_status(health_status_t* out_status) {
    if (!out_status) return ESP_ERR_INVALID_ARG;

    out_status->free_heap = esp_get_free_heap_size();
    out_status->min_free_heap = esp_get_minimum_free_heap_size();
    
    #if CONFIG_SPIRAM
    out_status->psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    #else
    out_status->psram_free = 0;
    #endif

    // Uptime (approximate)
    out_status->uptime_seconds = (xTaskGetTickCount() * portTICK_PERIOD_MS) / 1000;

    // CPU load estimation (simplified)
    out_status->cpu_load_percent = 0; // Would need proper implementation

    // Determine health
    out_status->is_healthy = 
        (out_status->free_heap > HEAP_CRITICAL_THRESHOLD) &&
        (out_status->psram_free > PSRAM_CRITICAL_THRESHOLD || !CONFIG_SPIRAM);

    return ESP_OK;
}

esp_err_t health_watch_task(const char* task_name) {
    if (!s_initialized || s_watchdog_timeout_ms == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    TaskHandle_t task = xTaskGetHandle(task_name);
    if (!task) {
        ESP_LOGW(TAG, "Task '%s' not found", task_name);
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t err = esp_task_wdt_add(task);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Watching task: %s", task_name);
    }
    return err;
}

esp_err_t health_unwatch_task(const char* task_name) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    TaskHandle_t task = xTaskGetHandle(task_name);
    if (!task) return ESP_ERR_NOT_FOUND;

    return esp_task_wdt_delete(task);
}

void health_feed_watchdog(void) {
    if (s_watchdog_timeout_ms > 0) {
        esp_task_wdt_reset();
    }
}

void health_reboot_system(void) {
    ESP_LOGW(TAG, "System reboot triggered");
    esp_restart();
}

uint32_t health_get_stack_hwm(void) {
    return uxTaskGetStackHighWaterMark(NULL);
}
