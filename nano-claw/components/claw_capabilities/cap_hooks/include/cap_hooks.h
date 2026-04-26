#pragma once
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HOOK_MAX_CHAIN 16
#define HOOK_MAX_DATA_LEN 1024

typedef enum {
    HOOK_EVENT_PRE_REQUEST,
    HOOK_EVENT_POST_RESPONSE,
    HOOK_EVENT_ERROR,
    HOOK_EVENT_CONNECTED,
    HOOK_EVENT_DISCONNECTED,
    HOOK_EVENT_MESSAGE_RECEIVED,
    HOOK_EVENT_MESSAGE_SENT,
    HOOK_EVENT_CUSTOM
} hook_event_t;

typedef enum {
    HOOK_PRIORITY_CRITICAL = 0,
    HOOK_PRIORITY_HIGH = 1,
    HOOK_PRIORITY_NORMAL = 5,
    HOOK_PRIORITY_LOW = 9,
    HOOK_PRIORITY_BACKGROUND = 10
} hook_priority_t;

typedef enum {
    HOOK_CONTINUE,
    HOOK_ABORT,
    HOOK_SKIP_REMAINING
} hook_result_t;

typedef struct {
    char *data;
    size_t length;
    void *context;
} hook_data_t;

typedef hook_result_t (*hook_callback_t)(hook_event_t event, hook_data_t *data);

esp_err_t hooks_init(void);
esp_err_t hooks_register(hook_event_t event, hook_priority_t priority, hook_callback_t callback);
esp_err_t hooks_unregister(hook_event_t event, hook_callback_t callback);
esp_err_t hooks_trigger(hook_event_t event, hook_data_t *data);
int hooks_get_count(hook_event_t event);

#ifdef __cplusplus
}
#endif
