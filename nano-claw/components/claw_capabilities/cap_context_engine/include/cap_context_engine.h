#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO: Implement cap_context_engine functionality
// Based on OpenClaw/zclaw/mimiclaw patterns

esp_err_t cap_context_engine_init(void);
esp_err_t cap_context_engine_deinit(void);

#ifdef __cplusplus
}
#endif
