#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO: Implement cap_ratelimit functionality
// Based on OpenClaw/zclaw/mimiclaw patterns

esp_err_t cap_ratelimit_init(void);
esp_err_t cap_ratelimit_deinit(void);

#ifdef __cplusplus
}
#endif
