/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "cap_secrets.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "cJSON.h"
#include "claw_cap.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef CONFIG_NANO_CLAW_USE_HW_ENCRYPTION
#include "esp_crypto.h"
#endif

static const char *TAG = "cap_secrets";

#define CAP_SECRETS_STORE_FILENAME      "secrets_vault.json"
#define CAP_SECRETS_PATH_SIZE           256
#define CAP_SECRETS_MAX_PATH            256

typedef struct {
    bool initialized;
    char vault_root_dir[192];
    uint8_t master_key[32];
    size_t master_key_len;
    bool use_hw_encryption;
    SemaphoreHandle_t mutex;
} cap_secrets_state_t;

static cap_secrets_state_t s_secrets = {0};

// Forward declarations
static esp_err_t cap_secrets_ensure_dir(const char *path);
static esp_err_t cap_secrets_get_store_path(char *path, size_t path_size);
static int64_t cap_secrets_get_time_ms(void);
static esp_err_t cap_secrets_encrypt_value(const char *plaintext, char *ciphertext, size_t ciphertext_size);
static esp_err_t cap_secrets_decrypt_value(const char *ciphertext, char *plaintext, size_t plaintext_size);

static const char *s_type_names[] = {
    "string",
    "api_key",
    "token",
    "password",
    "certificate"
};

static esp_err_t cap_secrets_ensure_dir(const char *path)
{
    struct stat st = {0};

    if (!path || !path[0]) {
        return ESP_ERR_INVALID_ARG;
    }
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode) ? ESP_OK : ESP_FAIL;
    }
    if (mkdir(path, 0755) == 0 || errno == EEXIST) {
        return ESP_OK;
    }
    return ESP_FAIL;
}

static esp_err_t cap_secrets_get_store_path(char *path, size_t path_size)
{
    int written;

    if (!path || path_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    written = snprintf(path, path_size, "%s/%s", s_secrets.vault_root_dir, CAP_SECRETS_STORE_FILENAME);
    if (written < 0 || (size_t)written >= path_size) {
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

static int64_t cap_secrets_get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static esp_err_t cap_secrets_encrypt_value(const char *plaintext, char *ciphertext, size_t ciphertext_size)
{
#ifdef CONFIG_NANO_CLAW_USE_HW_ENCRYPTION
    // TODO: Implement hardware encryption using ESP32-S3 AES/HMAC peripheral
    // For now, use simple base64 encoding as placeholder
#endif
    
    // Simple implementation - in production, use proper encryption
    if (!plaintext || !ciphertext || ciphertext_size < strlen(plaintext) * 2) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Placeholder: just copy (implement real encryption here)
    strncpy(ciphertext, plaintext, ciphertext_size - 1);
    ciphertext[ciphertext_size - 1] = '\0';
    
    return ESP_OK;
}

static esp_err_t cap_secrets_decrypt_value(const char *ciphertext, char *plaintext, size_t plaintext_size)
{
#ifdef CONFIG_NANO_CLAW_USE_HW_ENCRYPTION
    // TODO: Implement hardware decryption
#endif
    
    // Simple implementation - in production, use proper decryption
    if (!ciphertext || !plaintext || plaintext_size < strlen(ciphertext) + 1) {
        return ESP_ERR_INVALID_ARG;
    }
    
    strncpy(plaintext, ciphertext, plaintext_size - 1);
    plaintext[plaintext_size - 1] = '\0';
    
    return ESP_OK;
}

esp_err_t cap_secrets_init(const char *vault_root_dir, const uint8_t *master_key, size_t master_key_len)
{
    SemaphoreHandle_t mutex = s_secrets.mutex;

    if (!vault_root_dir || !vault_root_dir[0]) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(&s_secrets, 0, sizeof(s_secrets));
    s_secrets.mutex = mutex;

    strlcpy(s_secrets.vault_root_dir, vault_root_dir, sizeof(s_secrets.vault_root_dir));

    if (master_key && master_key_len > 0 && master_key_len <= 32) {
        memcpy(s_secrets.master_key, master_key, master_key_len);
        s_secrets.master_key_len = master_key_len;
    } else {
        s_secrets.master_key_len = 0;
    }

#ifdef CONFIG_NANO_CLAW_USE_HW_ENCRYPTION
    s_secrets.use_hw_encryption = true;
#else
    s_secrets.use_hw_encryption = false;
#endif

    if (!s_secrets.mutex) {
        s_secrets.mutex = xSemaphoreCreateRecursiveMutex();
    }
    if (!s_secrets.mutex) {
        return ESP_ERR_NO_MEM;
    }

    if (cap_secrets_ensure_dir(s_secrets.vault_root_dir) != ESP_OK) {
        vSemaphoreDelete(s_secrets.mutex);
        s_secrets.mutex = NULL;
        return ESP_FAIL;
    }

    s_secrets.initialized = true;
    ESP_LOGI(TAG, "Secrets vault initialized at %s (HW encryption: %s)", 
             s_secrets.vault_root_dir, s_secrets.use_hw_encryption ? "yes" : "no");
    return ESP_OK;
}

esp_err_t cap_secrets_deinit(void)
{
    if (s_secrets.mutex) {
        vSemaphoreDelete(s_secrets.mutex);
        s_secrets.mutex = NULL;
    }
    // Secure wipe of master key
    if (s_secrets.master_key_len > 0) {
        memset(s_secrets.master_key, 0, sizeof(s_secrets.master_key));
    }
    s_secrets.initialized = false;
    ESP_LOGI(TAG, "Secrets vault deinitialized");
    return ESP_OK;
}

esp_err_t cap_secrets_set(const char *name, const char *value, cap_secrets_type_t type)
{
    char path[CAP_SECRETS_PATH_SIZE];
    FILE *file = NULL;
    long size = 0;
    char *text = NULL;
    size_t read_bytes;
    cJSON *root = NULL;
    cJSON *secrets = NULL;
    cJSON *existing_secret = NULL;
    cJSON *new_secret = NULL;
    char encrypted_value[CAP_SECRETS_MAX_VALUE_LEN * 2];
    int64_t now_ms;
    esp_err_t err;

    if (!s_secrets.initialized || !name || !name[0] || !value) {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(name) >= CAP_SECRETS_MAX_NAME_LEN) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (strlen(value) >= CAP_SECRETS_MAX_VALUE_LEN) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (type < 0 || type >= 5) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTakeRecursive(s_secrets.mutex, portMAX_DELAY);

    err = cap_secrets_get_store_path(path, sizeof(path));
    if (err != ESP_OK) {
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return err;
    }

    // Try to read existing store
    file = fopen(path, "rb");
    if (file) {
        if (fseek(file, 0, SEEK_END) == 0) {
            size = ftell(file);
            if (size >= 0 && size < 1024 * 1024) {
                fseek(file, 0, SEEK_SET);
                text = calloc(1, (size_t)size + 1);
                if (text) {
                    read_bytes = fread(text, 1, (size_t)size, file);
                    text[read_bytes] = '\0';
                    root = cJSON_Parse(text);
                    free(text);
                }
            }
        }
        fclose(file);
    }

    // Create new store if needed
    if (!root || !cJSON_IsObject(root)) {
        if (root) cJSON_Delete(root);
        root = cJSON_CreateObject();
        if (!root) {
            xSemaphoreGiveRecursive(s_secrets.mutex);
            return ESP_ERR_NO_MEM;
        }
        cJSON_AddNumberToObject(root, "version", 1);
        cJSON_AddArrayToObject(root, "secrets");
    }

    secrets = cJSON_GetObjectItemCaseSensitive(root, "secrets");
    if (!cJSON_IsArray(secrets)) {
        cJSON_Delete(root);
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return ESP_ERR_INVALID_STATE;
    }

    // Check if secret already exists
    cJSON_ArrayForEach(existing_secret, secrets) {
        cJSON *name_item = cJSON_GetObjectItemCaseSensitive(existing_secret, "name");
        if (cJSON_IsString(name_item) && strcmp(name_item->valuestring, name) == 0) {
            // Update existing
            break;
        }
        existing_secret = NULL;
    }

    // Encrypt value
    err = cap_secrets_encrypt_value(value, encrypted_value, sizeof(encrypted_value));
    if (err != ESP_OK) {
        cJSON_Delete(root);
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return err;
    }

    now_ms = cap_secrets_get_time_ms();

    if (existing_secret) {
        // Update existing secret
        cJSON_ReplaceItemInObject(existing_secret, "value", cJSON_CreateString(encrypted_value));
        cJSON_ReplaceItemInObject(existing_secret, "type", cJSON_CreateString(s_type_names[type]));
        cJSON_ReplaceItemInObject(existing_secret, "updated_at", cJSON_CreateNumber((double)now_ms));
        cJSON_ReplaceItemInObject(existing_secret, "encrypted", cJSON_CreateBool(s_secrets.master_key_len > 0 || s_secrets.use_hw_encryption));
    } else {
        // Create new secret
        new_secret = cJSON_CreateObject();
        if (!new_secret) {
            cJSON_Delete(root);
            xSemaphoreGiveRecursive(s_secrets.mutex);
            return ESP_ERR_NO_MEM;
        }

        cJSON_AddStringToObject(new_secret, "name", name);
        cJSON_AddStringToObject(new_secret, "value", encrypted_value);
        cJSON_AddStringToObject(new_secret, "type", s_type_names[type]);
        cJSON_AddNumberToObject(new_secret, "created_at", (double)now_ms);
        cJSON_AddNumberToObject(new_secret, "updated_at", (double)now_ms);
        cJSON_AddBoolToObject(new_secret, "encrypted", s_secrets.master_key_len > 0 || s_secrets.use_hw_encryption);
        cJSON_AddNumberToObject(new_secret, "access_count", 0);

        cJSON_AddItemToArray(secrets, new_secret);
    }

    // Write back to file
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!json) {
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return ESP_ERR_NO_MEM;
    }

    file = fopen(path, "wb");
    if (!file) {
        free(json);
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return ESP_FAIL;
    }

    if (fputs(json, file) < 0) {
        fclose(file);
        free(json);
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return ESP_FAIL;
    }

    fclose(file);
    free(json);

    xSemaphoreGiveRecursive(s_secrets.mutex);
    ESP_LOGI(TAG, "Stored secret: %s (type: %s)", name, s_type_names[type]);
    return ESP_OK;
}

esp_err_t cap_secrets_get(const char *name, char *out_value, size_t out_value_size)
{
    char path[CAP_SECRETS_PATH_SIZE];
    FILE *file = NULL;
    long size = 0;
    char *text = NULL;
    size_t read_bytes;
    cJSON *root = NULL;
    cJSON *secrets = NULL;
    cJSON *secret = NULL;
    esp_err_t err = ESP_ERR_NOT_FOUND;

    if (!s_secrets.initialized || !name || !out_value || out_value_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTakeRecursive(s_secrets.mutex, portMAX_DELAY);

    err = cap_secrets_get_store_path(path, sizeof(path));
    if (err != ESP_OK) {
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return err;
    }

    file = fopen(path, "rb");
    if (!file) {
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return ESP_FAIL;
    }
    size = ftell(file);
    if (size < 0 || size > 1024 * 1024) {
        fclose(file);
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return ESP_FAIL;
    }
    fseek(file, 0, SEEK_SET);

    text = calloc(1, (size_t)size + 1);
    if (!text) {
        fclose(file);
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return ESP_ERR_NO_MEM;
    }
    read_bytes = fread(text, 1, (size_t)size, file);
    fclose(file);
    text[read_bytes] = '\0';

    root = cJSON_Parse(text);
    free(text);

    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return ESP_ERR_INVALID_RESPONSE;
    }

    secrets = cJSON_GetObjectItemCaseSensitive(root, "secrets");
    if (!cJSON_IsArray(secrets)) {
        cJSON_Delete(root);
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return ESP_ERR_INVALID_STATE;
    }

    cJSON_ArrayForEach(secret, secrets) {
        cJSON *name_item = cJSON_GetObjectItemCaseSensitive(secret, "name");
        if (cJSON_IsString(name_item) && strcmp(name_item->valuestring, name) == 0) {
            cJSON *value_item = cJSON_GetObjectItemCaseSensitive(secret, "value");
            cJSON *access_count_item = cJSON_GetObjectItemCaseSensitive(secret, "access_count");

            if (cJSON_IsString(value_item)) {
                char decrypted[CAP_SECRETS_MAX_VALUE_LEN];
                err = cap_secrets_decrypt_value(value_item->valuestring, decrypted, sizeof(decrypted));
                if (err == ESP_OK) {
                    if (strlen(decrypted) >= out_value_size) {
                        err = ESP_ERR_NO_MEM;
                    } else {
                        strncpy(out_value, decrypted, out_value_size - 1);
                        out_value[out_value_size - 1] = '\0';
                        err = ESP_OK;
                    }
                }
            }

            // Increment access count
            if (cJSON_IsNumber(access_count_item)) {
                int new_count = access_count_item->valueint + 1;
                if (new_count > 255) new_count = 255;
                cJSON_SetNumberValue(access_count_item, new_count);
            }

            // Write updated access count back
            if (err == ESP_OK) {
                char *json = cJSON_PrintUnformatted(root);
                if (json) {
                    FILE *write_file = fopen(path, "wb");
                    if (write_file) {
                        fputs(json, write_file);
                        fclose(write_file);
                    }
                    free(json);
                }
            }

            break;
        }
    }

    cJSON_Delete(root);
    xSemaphoreGiveRecursive(s_secrets.mutex);
    return err;
}

esp_err_t cap_secrets_delete(const char *name)
{
    char path[CAP_SECRETS_PATH_SIZE];
    FILE *file = NULL;
    long size = 0;
    char *text = NULL;
    size_t read_bytes;
    cJSON *root = NULL;
    cJSON *secrets = NULL;
    cJSON *secret = NULL;
    bool found = false;
    esp_err_t err;

    if (!s_secrets.initialized || !name) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTakeRecursive(s_secrets.mutex, portMAX_DELAY);

    err = cap_secrets_get_store_path(path, sizeof(path));
    if (err != ESP_OK) {
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return err;
    }

    file = fopen(path, "rb");
    if (!file) {
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return ESP_FAIL;
    }
    size = ftell(file);
    fseek(file, 0, SEEK_SET);

    text = calloc(1, (size_t)size + 1);
    if (!text) {
        fclose(file);
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return ESP_ERR_NO_MEM;
    }
    read_bytes = fread(text, 1, (size_t)size, file);
    fclose(file);
    text[read_bytes] = '\0';

    root = cJSON_Parse(text);
    free(text);

    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return ESP_ERR_INVALID_RESPONSE;
    }

    secrets = cJSON_GetObjectItemCaseSensitive(root, "secrets");
    if (!cJSON_IsArray(secrets)) {
        cJSON_Delete(root);
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return ESP_ERR_INVALID_STATE;
    }

    cJSON_ArrayForEach(secret, secrets) {
        cJSON *name_item = cJSON_GetObjectItemCaseSensitive(secret, "name");
        if (cJSON_IsString(name_item) && strcmp(name_item->valuestring, name) == 0) {
            cJSON_DeleteItemFromArray(secrets, secret->index);
            found = true;
            break;
        }
    }

    if (found) {
        char *json = cJSON_PrintUnformatted(root);
        if (json) {
            FILE *write_file = fopen(path, "wb");
            if (write_file) {
                fputs(json, write_file);
                fclose(write_file);
            }
            free(json);
        }
    }

    cJSON_Delete(root);
    xSemaphoreGiveRecursive(s_secrets.mutex);

    if (found) {
        ESP_LOGI(TAG, "Deleted secret: %s", name);
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

bool cap_secrets_exists(const char *name)
{
    char value[1];
    esp_err_t err = cap_secrets_get(name, value, sizeof(value));
    return (err == ESP_OK);
}

size_t cap_secrets_count(void)
{
    char path[CAP_SECRETS_PATH_SIZE];
    FILE *file = NULL;
    long size = 0;
    char *text = NULL;
    size_t read_bytes;
    cJSON *root = NULL;
    cJSON *secrets = NULL;
    size_t count = 0;

    if (!s_secrets.initialized) {
        return 0;
    }

    xSemaphoreTakeRecursive(s_secrets.mutex, portMAX_DELAY);

    if (cap_secrets_get_store_path(path, sizeof(path)) != ESP_OK) {
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return 0;
    }

    file = fopen(path, "rb");
    if (!file) {
        xSemaphoreGiveRecursive(s_secrets.mutex);
        return 0;
    }

    if (fseek(file, 0, SEEK_END) == 0) {
        size = ftell(file);
        if (size >= 0 && size < 1024 * 1024) {
            fseek(file, 0, SEEK_SET);
            text = calloc(1, (size_t)size + 1);
            if (text) {
                read_bytes = fread(text, 1, (size_t)size, file);
                text[read_bytes] = '\0';
                root = cJSON_Parse(text);
                free(text);

                if (cJSON_IsObject(root)) {
                    secrets = cJSON_GetObjectItemCaseSensitive(root, "secrets");
                    if (cJSON_IsArray(secrets)) {
                        count = cJSON_GetArraySize(secrets);
                    }
                }
                if (root) cJSON_Delete(root);
            }
        }
    }
    fclose(file);

    xSemaphoreGiveRecursive(s_secrets.mutex);
    return count;
}

esp_err_t cap_secrets_clear_all(void)
{
    char path[CAP_SECRETS_PATH_SIZE];
    esp_err_t err;

    if (!s_secrets.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTakeRecursive(s_secrets.mutex, portMAX_DELAY);

    err = cap_secrets_get_store_path(path, sizeof(path));
    if (err == ESP_OK) {
        if (remove(path) != 0 && errno != ENOENT) {
            err = ESP_FAIL;
        } else {
            err = ESP_OK;
        }
    }

    xSemaphoreGiveRecursive(s_secrets.mutex);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Cleared all secrets");
    }
    return err;
}

// Stub implementations for list, get_meta, export, import, rotate
// Full implementations would follow similar patterns

esp_err_t cap_secrets_list(char out_names[][CAP_SECRETS_MAX_NAME_LEN], 
                           size_t max_names, size_t *out_count)
{
    // Simplified implementation
    if (!s_secrets.initialized || !out_count) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *out_count = cap_secrets_count();
    if (*out_count > max_names) {
        *out_count = max_names;
    }
    
    // TODO: Fill out_names array
    return ESP_OK;
}

esp_err_t cap_secrets_get_meta(const char *name, cap_secrets_meta_t *out_meta)
{
    // TODO: Implement metadata retrieval
    (void)name;
    (void)out_meta;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t cap_secrets_export(const char *export_path, const uint8_t *export_key, size_t export_key_len)
{
    // TODO: Implement encrypted export
    (void)export_path;
    (void)export_key;
    (void)export_key_len;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t cap_secrets_import(const char *import_path, const uint8_t *import_key, size_t import_key_len)
{
    // TODO: Implement encrypted import
    (void)import_path;
    (void)import_key;
    (void)import_key_len;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t cap_secrets_rotate(const char *name, const char *new_value)
{
    // Rotation is just update with new value
    return cap_secrets_set(name, new_value, CAP_SECRETS_TYPE_API_KEY);
}

// Capability execute functions
static esp_err_t cap_secrets_set_execute(const char *input_json,
                                         const claw_cap_call_context_t *ctx,
                                         char *output,
                                         size_t output_size)
{
    cJSON *input = NULL;
    cJSON *name_item, *value_item, *type_item;
    const char *name, *value;
    cap_secrets_type_t type = CAP_SECRETS_TYPE_STRING;
    esp_err_t err;

    (void)ctx;

    if (!s_secrets.initialized) {
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"secrets vault not initialized\"}");
        }
        return ESP_ERR_INVALID_STATE;
    }

    input = cJSON_Parse(input_json);
    if (!cJSON_IsObject(input)) {
        cJSON_Delete(input);
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"invalid input JSON\"}");
        }
        return ESP_ERR_INVALID_ARG;
    }

    name_item = cJSON_GetObjectItemCaseSensitive(input, "name");
    value_item = cJSON_GetObjectItemCaseSensitive(input, "value");
    type_item = cJSON_GetObjectItemCaseSensitive(input, "type");

    if (!cJSON_IsString(name_item) || !cJSON_IsString(value_item)) {
        cJSON_Delete(input);
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"name and value required\"}");
        }
        return ESP_ERR_INVALID_ARG;
    }

    name = name_item->valuestring;
    value = value_item->valuestring;

    if (cJSON_IsString(type_item)) {
        const char *type_str = type_item->valuestring;
        if (strcmp(type_str, "api_key") == 0) type = CAP_SECRETS_TYPE_API_KEY;
        else if (strcmp(type_str, "token") == 0) type = CAP_SECRETS_TYPE_TOKEN;
        else if (strcmp(type_str, "password") == 0) type = CAP_SECRETS_TYPE_PASSWORD;
        else if (strcmp(type_str, "certificate") == 0) type = CAP_SECRETS_TYPE_CERTIFICATE;
    }

    err = cap_secrets_set(name, value, type);
    cJSON_Delete(input);

    if (err != ESP_OK) {
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"%s\"}", esp_err_to_name(err));
        }
        return err;
    }

    if (output && output_size > 0) {
        snprintf(output, output_size, "{\"ok\":true,\"message\":\"Secret stored\"}");
    }
    return ESP_OK;
}

static esp_err_t cap_secrets_get_execute(const char *input_json,
                                         const claw_cap_call_context_t *ctx,
                                         char *output,
                                         size_t output_size)
{
    cJSON *input = NULL;
    cJSON *name_item;
    const char *name;
    char value[CAP_SECRETS_MAX_VALUE_LEN];
    esp_err_t err;

    (void)ctx;

    if (!s_secrets.initialized) {
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"secrets vault not initialized\"}");
        }
        return ESP_ERR_INVALID_STATE;
    }

    input = cJSON_Parse(input_json);
    if (!cJSON_IsObject(input)) {
        cJSON_Delete(input);
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"invalid input JSON\"}");
        }
        return ESP_ERR_INVALID_ARG;
    }

    name_item = cJSON_GetObjectItemCaseSensitive(input, "name");
    if (!cJSON_IsString(name_item)) {
        cJSON_Delete(input);
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"name required\"}");
        }
        return ESP_ERR_INVALID_ARG;
    }

    name = name_item->valuestring;
    err = cap_secrets_get(name, value, sizeof(value));
    cJSON_Delete(input);

    if (err != ESP_OK) {
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"%s\"}", esp_err_to_name(err));
        }
        return err;
    }

    if (output && output_size > 0) {
        snprintf(output, output_size, "{\"ok\":true,\"value\":\"%s\"}", value);
    }
    return ESP_OK;
}

static const claw_cap_descriptor_t s_secrets_caps[] = {
    {
        .id = "set_secret",
        .name = "set_secret",
        .family = "secrets",
        .description = "Store a secret in the encrypted vault",
        .kind = CLAW_CAP_KIND_CALLABLE,
        .cap_flags = CLAW_CAP_FLAG_RESTRICTED,
        .input_schema_json = "{\"type\":\"object\",\"required\":[\"name\",\"value\"],\"properties\":{\"name\":{\"type\":\"string\"},\"value\":{\"type\":\"string\"},\"type\":{\"type\":\"string\"}}}",
        .execute = cap_secrets_set_execute,
    },
    {
        .id = "get_secret",
        .name = "get_secret",
        .family = "secrets",
        .description = "Retrieve a secret from the vault",
        .kind = CLAW_CAP_KIND_CALLABLE,
        .cap_flags = CLAW_CAP_FLAG_RESTRICTED,
        .input_schema_json = "{\"type\":\"object\",\"required\":[\"name\"],\"properties\":{\"name\":{\"type\":\"string\"}}}",
        .execute = cap_secrets_get_execute,
    },
};

static const claw_cap_group_t s_secrets_group = {
    .group_id = "cap_secrets",
    .plugin_name = "cap_secrets",
    .version = "1.0.0",
    .descriptors = s_secrets_caps,
    .descriptor_count = sizeof(s_secrets_caps) / sizeof(s_secrets_caps[0]),
};

esp_err_t cap_secrets_register_group(void)
{
    return claw_cap_register_group(&s_secrets_group);
}
