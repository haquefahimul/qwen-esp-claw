/*
 * Nano-Claw Heartbeat Monitor Capability
 * Copyright 2024 Nano-Claw Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CAP_HEARTBEAT_H
#define CAP_HEARTBEAT_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HEARTBEAT_MIN_INTERVAL_SEC 1
#define HEARTBEAT_MAX_INTERVAL_SEC 86400

typedef enum {
    HEARTBEAT_STATUS_STOPPED,
    HEARTBEAT_STATUS_RUNNING,
    HEARTBEAT_STATUS_CONNECTED,
    HEARTBEAT_STATUS_DISCONNECTED,
    HEARTBEAT_STATUS_ERROR
} heartbeat_status_t;

typedef void (*heartbeat_callback_t)(bool success, void *user_data);

typedef struct {
    uint32_t interval_sec;
    heartbeat_callback_t callback;
    void *user_data;
    bool auto_reconnect;
    uint32_t max_retries;
    uint32_t retry_count;
} heartbeat_config_t;

esp_err_t heartbeat_init(void);
esp_err_t heartbeat_start(heartbeat_config_t *config);
esp_err_t heartbeat_stop(void);
esp_err_t heartbeat_set_interval(uint32_t interval_sec);
heartbeat_status_t heartbeat_get_status(void);
uint32_t heartbeat_get_uptime(void);
uint32_t heartbeat_get_beat_count(void);
void heartbeat_get_stats(uint32_t *total_beats, uint32_t *failures, 
                         uint32_t *reconnects);

#ifdef __cplusplus
}
#endif

#endif
