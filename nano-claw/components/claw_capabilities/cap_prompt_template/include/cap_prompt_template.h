#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO: Implement cap_prompt_template functionality
// Based on OpenClaw/zclaw/mimiclaw patterns

esp_err_t cap_prompt_template_init(void);
esp_err_t cap_prompt_template_deinit(void);

#ifdef __cplusplus
}
#endif
