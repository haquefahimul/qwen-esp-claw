#pragma once
#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAIRING_CODE_LEN 9
#define MAX_PENDING_PAIRINGS 3

typedef enum {
    CHANNEL_DISCORD,
    CHANNEL_TELEGRAM,
    CHANNEL_WECHAT,
    CHANNEL_QQ,
    CHANNEL_FEISHU
} pairing_channel_t;

typedef struct {
    char code[PAIRING_CODE_LEN];
    pairing_channel_t channel;
    uint64_t expiry_timestamp;
    bool used;
} pairing_request_t;

esp_err_t pairing_init(void);
esp_err_t pairing_generate_code(pairing_channel_t channel, char *code, size_t len);
esp_err_t pairing_accept_code(const char *code, const char *device_id);
esp_err_t pairing_get_status(const char *code, bool *is_valid, bool *is_used);
esp_err_t pairing_cleanup_expired(void);
int pairing_get_pending_count(pairing_channel_t channel);

#ifdef __cplusplus
}
#endif
