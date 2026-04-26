/**
 * @file cap_health.h
 * @brief Task Watchdog & System Health Monitor for Nano-Claw
 * 
 * Monitors task stack usage, CPU load, and free heap.
 * Triggers safe reboot if critical thresholds exceeded.
 */

#ifndef CAP_HEALTH_H
#define CAP_HEALTH_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Health status structure
 */
typedef struct {
    uint32_t free_heap;
    uint32_t min_free_heap;
    uint32_t psram_free;
    uint32_t cpu_load_percent;
    uint32_t uptime_seconds;
    bool is_healthy;
} health_status_t;

/**
 * @brief Initialize health monitoring subsystem
 * @param watchdog_timeout_ms Timeout in ms (0 to disable watchdog)
 * @return ESP_OK on success
 */
esp_err_t health_init(uint32_t watchdog_timeout_ms);

/**
 * @brief Start the health monitor task
 * @param check_interval_ms Interval between checks
 * @return ESP_OK on success
 */
esp_err_t health_start_monitor(uint32_t check_interval_ms);

/**
 * @brief Stop the health monitor task
 */
void health_stop_monitor(void);

/**
 * @brief Get current system health status
 * @param out_status Pointer to store status
 * @return ESP_OK on success
 */
esp_err_t health_get_status(health_status_t* out_status);

/**
 * @brief Register a task for watchdog monitoring
 * @param task_name Name of task to monitor
 * @return ESP_OK on success
 */
esp_err_t health_watch_task(const char* task_name);

/**
 * @brief Unregister a task from watchdog monitoring
 * @param task_name Name of task
 * @return ESP_OK on success
 */
esp_err_t health_unwatch_task(const char* task_name);

/**
 * @brief Feed the watchdog (call periodically from monitored tasks)
 */
void health_feed_watchdog(void);

/**
 * @brief Trigger a safe system reboot
 */
void health_reboot_system(void);

/**
 * @brief Get stack high water mark for current task
 * @return Stack bytes remaining
 */
uint32_t health_get_stack_hwm(void);

#ifdef __cplusplus
}
#endif

#endif // CAP_HEALTH_H
