#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO: Implement cap_watchdog functionality
// Based on OpenClaw/zclaw/mimiclaw patterns

esp_err_t cap_watchdog_init(void);
esp_err_t cap_watchdog_deinit(void);

#ifdef __cplusplus
}
#endif
