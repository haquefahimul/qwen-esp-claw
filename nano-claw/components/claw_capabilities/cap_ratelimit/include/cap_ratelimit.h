/* Nano-Claw Rate Limiter - Copyright 2024 */
#ifndef CAP_RATELIMIT_H
#define CAP_RATELIMIT_H
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t window_ms;
    uint32_t max_requests;
    uint32_t burst;
} ratelimit_config_t;

esp_err_t ratelimit_init(void);
esp_err_t ratelimit_create(const char *name, ratelimit_config_t *cfg);
esp_err_t ratelimit_check(const char *name, bool *allowed);
esp_err_t ratelimit_reset(const char *name);
void ratelimit_cleanup(void);

#ifdef __cplusplus
}
#endif
#endif
