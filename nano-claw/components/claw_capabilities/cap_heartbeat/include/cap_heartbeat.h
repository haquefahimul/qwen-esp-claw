#pragma once
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*heartbeat_callback_t)(void *arg);

typedef struct {
    uint32_t interval_ms;
    heartbeat_callback_t callback;
    void *arg;
    bool enabled;
} heartbeat_config_t;

esp_err_t heartbeat_init(heartbeat_config_t *config);
esp_err_t heartbeat_start(void);
esp_err_t heartbeat_stop(void);
esp_err_t heartbeat_set_interval(uint32_t interval_ms);
esp_err_t heartbeat_send_now(void);

#ifdef __cplusplus
}
#endif
