#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO: Implement cap_react_agent functionality
// Based on OpenClaw/zclaw/mimiclaw patterns

esp_err_t cap_react_agent_init(void);
esp_err_t cap_react_agent_deinit(void);

#ifdef __cplusplus
}
#endif
