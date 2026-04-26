#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO: Implement cap_vector_store functionality
// Based on OpenClaw/zclaw/mimiclaw patterns

esp_err_t cap_vector_store_init(void);
esp_err_t cap_vector_store_deinit(void);

#ifdef __cplusplus
}
#endif
