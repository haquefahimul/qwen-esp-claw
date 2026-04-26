/*
 * Nano-Claw Cron Scheduler Capability
 * 
 * Implements cron-based job scheduling for ESP32-S3
 * Features: One-shot, recurring, cron expressions
 * 
 * Copyright 2024 Nano-Claw Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CAP_CRON_H
#define CAP_CRON_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of concurrent cron jobs
 */
#define CRON_MAX_JOBS CONFIG_CLAW_CRON_MAX_JOBS

/**
 * @brief Maximum length of job name
 */
#define CRON_JOB_NAME_MAX 32

/**
 * @brief Maximum length of cron expression
 */
#define CRON_EXPRESSION_MAX 64

/**
 * @brief Cron job types
 */
typedef enum {
    CRON_TYPE_ONESHOT,      ///< Execute once after delay
    CRON_TYPE_RECURRING,    ///< Execute at fixed intervals
    CRON_TYPE_CRON_EXPR     ///< Execute based on cron expression
} cron_job_type_t;

/**
 * @brief Cron job status
 */
typedef enum {
    CRON_STATUS_PENDING,    ///< Job scheduled, waiting to run
    CRON_STATUS_RUNNING,    ///< Job currently executing
    CRON_STATUS_COMPLETED,  ///< Job completed successfully
    CRON_STATUS_FAILED,     ///< Job execution failed
    CRON_STATUS_CANCELLED   ///< Job cancelled by user
} cron_job_status_t;

/**
 * @brief Callback function type for cron jobs
 * 
 * @param user_data User-defined data passed to callback
 */
typedef void (*cron_callback_t)(void *user_data);

/**
 * @brief Cron job structure
 */
typedef struct {
    char name[CRON_JOB_NAME_MAX];           ///< Job name
    cron_job_type_t type;                   ///< Job type
    char expression[CRON_EXPRESSION_MAX];   ///< Cron expression or interval
    cron_callback_t callback;               ///< Callback function
    void *user_data;                        ///< User data for callback
    cron_job_status_t status;               ///< Current status
    uint32_t interval_ms;                   ///< Interval in ms (for recurring)
    uint32_t delay_ms;                      ///< Delay for one-shot
    uint32_t last_run_time;                 ///< Last execution timestamp
    uint32_t next_run_time;                 ///< Next scheduled execution
    uint32_t run_count;                     ///< Number of times executed
    uint32_t max_runs;                      ///< Max runs (0 = infinite)
    bool enabled;                           ///< Job enabled flag
} cron_job_t;

/**
 * @brief Initialize cron scheduler
 * 
 * @return ESP_OK on success, ESP_ERR_NO_MEM if allocation fails
 */
esp_err_t cron_init(void);

/**
 * @brief Deinitialize cron scheduler and cancel all jobs
 * 
 * @return ESP_OK on success
 */
esp_err_t cron_deinit(void);

/**
 * @brief Schedule a one-shot job
 * 
 * @param name Job name (must be unique)
 * @param callback Callback function
 * @param user_data User data for callback
 * @param delay_ms Delay in milliseconds
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if invalid params,
 *         ESP_ERR_NO_MEM if max jobs reached
 */
esp_err_t cron_schedule_once(const char *name, cron_callback_t callback, 
                             void *user_data, uint32_t delay_ms);

/**
 * @brief Schedule a recurring job with fixed interval
 * 
 * @param name Job name (must be unique)
 * @param callback Callback function
 * @param user_data User data for callback
 * @param interval_ms Interval in milliseconds
 * @param max_runs Maximum number of runs (0 = infinite)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t cron_schedule_recurring(const char *name, cron_callback_t callback,
                                  void *user_data, uint32_t interval_ms, 
                                  uint32_t max_runs);

/**
 * @brief Schedule a job using cron expression
 * 
 * Supported formats:
 * - Standard: minute hour day month weekday
 * - Special: @hourly, @daily, @weekly, @monthly, @yearly
 * - Step values: */5 (every 5), 1-10/2 (every 2 from 1-10)
 * 
 * @param name Job name (must be unique)
 * @param expression Cron expression
 * @param callback Callback function
 * @param user_data User data for callback
 * @param max_runs Maximum number of runs (0 = infinite)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t cron_schedule_expression(const char *name, const char *expression,
                                   cron_callback_t callback, void *user_data,
                                   uint32_t max_runs);

/**
 * @brief Cancel a scheduled job
 * 
 * @param name Job name
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if job not found
 */
esp_err_t cron_cancel(const char *name);

/**
 * @brief Cancel all jobs
 * 
 * @return ESP_OK on success
 */
esp_err_t cron_cancel_all(void);

/**
 * @brief Get job status
 * 
 * @param name Job name
 * @param job Output job structure (can be NULL)
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if job not found
 */
esp_err_t cron_get_job(const char *name, cron_job_t *job);

/**
 * @brief Enable/disable a job
 * 
 * @param name Job name
 * @param enabled true to enable, false to disable
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if job not found
 */
esp_err_t cron_set_enabled(const char *name, bool enabled);

/**
 * @brief Get number of active jobs
 * 
 * @return Number of active jobs
 */
uint32_t cron_get_job_count(void);

/**
 * @brief Get job statistics
 * 
 * @param total_jobs Total number of jobs
 * @param running_jobs Number of currently running jobs
 * @param pending_jobs Number of pending jobs
 */
void cron_get_stats(uint32_t *total_jobs, uint32_t *running_jobs, 
                    uint32_t *pending_jobs);

#ifdef __cplusplus
}
#endif

#endif // CAP_CRON_H
