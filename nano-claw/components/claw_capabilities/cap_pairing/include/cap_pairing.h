/*
 * Nano-Claw Device Pairing Capability
 * Copyright 2024 Nano-Claw Contributors
 */

#ifndef CAP_PAIRING_H
#define CAP_PAIRING_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PAIRING_CODE_LENGTH 8
#define PAIRING_MAX_PENDING 3
#define PAIRING_TTL_SEC 3600

typedef enum {
    CHANNEL_DISCORD,
    CHANNEL_TELEGRAM,
    CHANNEL_WECHAT,
    CHANNEL_QQ,
    CHANNEL_FEISHU,
    CHANNEL_MAX
} pairing_channel_t;

typedef enum {
    PAIRING_STATUS_PENDING,
    PAIRING_STATUS_ACCEPTED,
    PAIRING_STATUS_EXPIRED,
    PAIRING_STATUS_REVOKED
} pairing_status_t;

typedef struct {
    char code[9];
    pairing_channel_t channel;
    pairing_status_t status;
    uint32_t created_at;
    uint32_t expires_at;
    char user_id[32];
} pairing_request_t;

esp_err_t pairing_init(void);
esp_err_t pairing_generate_code(pairing_channel_t channel, char *code_out);
esp_err_t pairing_accept(const char *code, const char *user_id);
esp_err_t pairing_revoke(const char *code);
esp_err_t pairing_get_request(const char *code, pairing_request_t *req);
esp_err_t pairing_cleanup_expired(void);
uint32_t pairing_get_pending_count(pairing_channel_t channel);

#ifdef __cplusplus
}
#endif

#endif
