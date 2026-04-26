/*
 * Nano-Claw Encrypted Secrets Vault
 * Copyright 2024 Nano-Claw Contributors
 */

#ifndef CAP_SECRETS_H
#define CAP_SECRETS_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SECRETS_MAX_ENTRIES CONFIG_CLAW_SECRETS_VAULT_SIZE
#define SECRET_NAME_MAX 32
#define SECRET_VALUE_MAX 256

typedef enum {
    SECRET_TYPE_STRING,
    SECRET_TYPE_API_KEY,
    SECRET_TYPE_TOKEN,
    SECRET_TYPE_PASSWORD,
    SECRET_TYPE_CERTIFICATE
} secret_type_t;

typedef struct {
    char name[SECRET_NAME_MAX];
    secret_type_t type;
    char value[SECRET_VALUE_MAX];
    uint32_t created_at;
    uint32_t updated_at;
    uint32_t access_count;
    bool enabled;
} secret_entry_t;

esp_err_t secrets_init(void);
esp_err_t secrets_set(const char *name, const char *value, secret_type_t type);
esp_err_t secrets_get(const char *name, char *value_out, size_t max_len);
esp_err_t secrets_delete(const char *name);
esp_err_t secrets_rotate(const char *name, const char *new_value);
esp_err_t secrets_wipe(void);
uint32_t secrets_count(void);

#ifdef __cplusplus
}
#endif

#endif
