#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO: Implement cap_event_bus functionality
// Based on OpenClaw/zclaw/mimiclaw patterns

esp_err_t cap_event_bus_init(void);
esp_err_t cap_event_bus_deinit(void);

#ifdef __cplusplus
}
#endif
