#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CRON_MAX_JOBS 32
#define CRON_MAX_EXPR_LEN 64

typedef int cron_job_id_t;

typedef enum {
    CRON_TYPE_ONCE,
    CRON_TYPE_RECURRING,
    CRON_TYPE_CRON_EXPR
} cron_job_type_t;

typedef void (*cron_callback_t)(void *arg);

typedef struct {
    cron_job_id_t id;
    cron_job_type_t type;
    char expression[CRON_MAX_EXPR_LEN];
    TickType_t interval_ticks;
    cron_callback_t callback;
    void *arg;
    TickType_t next_run;
    bool enabled;
} cron_job_t;

esp_err_t cron_init(void);
esp_err_t cron_deinit(void);
cron_job_id_t cron_schedule_once(uint32_t delay_seconds, cron_callback_t callback, void *arg);
cron_job_id_t cron_schedule_recurring(uint32_t interval_seconds, cron_callback_t callback, void *arg);
cron_job_id_t cron_schedule(const char *cron_expr, cron_callback_t callback, void *arg);
esp_err_t cron_cancel(cron_job_id_t job_id);
esp_err_t cron_enable(cron_job_id_t job_id);
esp_err_t cron_disable(cron_job_id_t job_id);
int cron_get_active_jobs(void);

#ifdef __cplusplus
}
#endif
