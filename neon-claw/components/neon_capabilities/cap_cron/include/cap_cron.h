/**
 * @file cap_cron.h
 * @brief Cron Scheduler for Neon-Claw
 * 
 * Implements cron expression parsing and job scheduling
 * Supports: */5 * * * *, @hourly, @daily, one-shot, recurring
 * 
 * Source: Ported from OpenClaw
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define CRON_MAX_JOBS 16
#define CRON_EXPR_LEN 32
#define CRON_DESC_LEN 64

typedef enum {
    CRON_ONCE,          // One-shot execution
    CRON_INTERVAL,      // Fixed interval (ms)
    CRON_EXPRESSION     // Cron expression string
} cron_type_t;

typedef void (*cron_callback_t)(void* arg);

typedef struct {
    char id[16];
    cron_type_t type;
    union {
        uint32_t interval_ms;
        char expression[CRON_EXPR_LEN];
    };
    cron_callback_t callback;
    void* arg;
    uint32_t last_run;
    uint32_t next_run;
    bool active;
    bool running;
} cron_job_t;

/**
 * @brief Initialize cron scheduler
 */
esp_err_t cron_init(void);

/**
 * @brief Deinitialize cron scheduler
 */
esp_err_t cron_deinit(void);

/**
 * @brief Add a one-shot job
 */
esp_err_t cron_add_once(const char* id, uint32_t delay_ms, cron_callback_t cb, void* arg);

/**
 * @brief Add a recurring job with fixed interval
 */
esp_err_t cron_add_interval(const char* id, uint32_t interval_ms, cron_callback_t cb, void* arg);

/**
 * @brief Add a job with cron expression
 * 
 * Supported formats:
 * - Standard: "*/5 * * * *" (minute hour dom month dow)
 * - Shortcuts: "@hourly", "@daily", "@reboot"
 */
esp_err_t cron_add_expression(const char* id, const char* expr, cron_callback_t cb, void* arg);

/**
 * @brief Remove a job by ID
 */
esp_err_t cron_remove(const char* id);

/**
 * @brief Pause a job
 */
esp_err_t cron_pause(const char* id);

/**
 * @brief Resume a paused job
 */
esp_err_t cron_resume(const char* id);

/**
 * @brief Get job status
 */
bool cron_is_active(const char* id);

/**
 * @brief Get next run time (Unix timestamp)
 */
uint32_t cron_get_next_run(const char* id);

/**
 * @brief Force immediate execution of a job
 */
esp_err_t cron_trigger_now(const char* id);

/**
 * @brief Get number of active jobs
 */
int cron_get_job_count(void);
