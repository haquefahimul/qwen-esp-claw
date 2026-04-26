/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "cap_pairing.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
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

static const char *TAG = "cap_pairing";

#define CAP_PAIRING_CODE_LENGTH      8
#define CAP_PAIRING_CODE_ALPHABET    "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"
#define CAP_PAIRING_PENDING_TTL_MS   (60 * 60 * 1000)  // 1 hour
#define CAP_PAIRING_PENDING_MAX      3
#define CAP_PAIRING_ID_SIZE          64
#define CAP_PAIRING_PATH_SIZE        256
#define CAP_PAIRING_KEY_SIZE         128

typedef struct {
    bool initialized;
    char pairing_root_dir[192];
    SemaphoreHandle_t mutex;
} cap_pairing_state_t;

static cap_pairing_state_t s_pairing = {0};

// Forward declarations
static esp_err_t cap_pairing_ensure_dir(const char *path);
static esp_err_t cap_pairing_get_channel_name(cap_pairing_channel_t channel, char *buf, size_t buf_size);
static esp_err_t cap_pairing_build_storage_path(cap_pairing_channel_t channel, char *path, size_t path_size);
static esp_err_t cap_pairing_read_store(cap_pairing_channel_t channel, cJSON **out_root);
static esp_err_t cap_pairing_write_store(cap_pairing_channel_t channel, cJSON *root);
static void cap_pairing_generate_random_id(char *buf, size_t buf_size);
static int64_t cap_pairing_get_time_ms(void);

static const char *s_channel_names[] = {
    "none",
    "discord",
    "telegram", 
    "wechat",
    "qq",
    "feishu"
};

static esp_err_t cap_pairing_ensure_dir(const char *path)
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

static esp_err_t cap_pairing_get_channel_name(cap_pairing_channel_t channel, char *buf, size_t buf_size)
{
    if (channel < 0 || channel >= CAP_PAIRING_CHANNEL_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!buf || buf_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    const char *name = s_channel_names[channel];
    if (strlen(name) >= buf_size) {
        return ESP_ERR_INVALID_SIZE;
    }
    strcpy(buf, name);
    return ESP_OK;
}

static esp_err_t cap_pairing_build_storage_path(cap_pairing_channel_t channel, char *path, size_t path_size)
{
    char channel_name[32];
    int written;
    esp_err_t err;

    err = cap_pairing_get_channel_name(channel, channel_name, sizeof(channel_name));
    if (err != ESP_OK) {
        return err;
    }

    written = snprintf(path, path_size, "%s/%s-pairing.json", 
                       s_pairing.pairing_root_dir, channel_name);
    if (written < 0 || (size_t)written >= path_size) {
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

static int64_t cap_pairing_get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void cap_pairing_generate_random_id(char *buf, size_t buf_size)
{
    static uint32_t counter = 0;
    uint32_t random_val;
    int64_t timestamp;
    
    if (buf_size < 17) {
        buf[0] = '\0';
        return;
    }

    timestamp = cap_pairing_get_time_ms();
    random_val = (uint32_t)(timestamp ^ (counter++));
    
    // Generate 16-char hex ID
    snprintf(buf, buf_size, "%08lx%08lx", 
             (unsigned long)(timestamp & 0xFFFFFFFF),
             (unsigned long)random_val);
}

static esp_err_t cap_pairing_read_store(cap_pairing_channel_t channel, cJSON **out_root)
{
    char path[CAP_PAIRING_PATH_SIZE];
    FILE *file = NULL;
    long size;
    char *text = NULL;
    size_t read_bytes;
    cJSON *root = NULL;
    esp_err_t err;

    err = cap_pairing_build_storage_path(channel, path, sizeof(path));
    if (err != ESP_OK) {
        return err;
    }

    file = fopen(path, "rb");
    if (!file) {
        // File doesn't exist yet, return empty store
        root = cJSON_CreateObject();
        if (!root) {
            return ESP_ERR_NO_MEM;
        }
        cJSON_AddNumberToObject(root, "version", 1);
        cJSON_AddArrayToObject(root, "requests");
        *out_root = root;
        return ESP_OK;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return ESP_FAIL;
    }
    size = ftell(file);
    if (size < 0 || size > 1024 * 1024) {  // Max 1MB
        fclose(file);
        return ESP_FAIL;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return ESP_FAIL;
    }

    text = calloc(1, (size_t)size + 1);
    if (!text) {
        fclose(file);
        return ESP_ERR_NO_MEM;
    }
    read_bytes = fread(text, 1, (size_t)size, file);
    fclose(file);
    text[read_bytes] = '\0';

    root = cJSON_Parse(text);
    free(text);
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Ensure required fields exist
    if (!cJSON_HasObjectItem(root, "version")) {
        cJSON_AddNumberToObject(root, "version", 1);
    }
    if (!cJSON_HasObjectItem(root, "requests") || !cJSON_IsArray(cJSON_GetObjectItem(root, "requests"))) {
        cJSON_Delete(cJSON_DetachItemFromObject(root, "requests"));
        cJSON_AddArrayToObject(root, "requests");
    }

    *out_root = root;
    return ESP_OK;
}

static esp_err_t cap_pairing_write_store(cap_pairing_channel_t channel, cJSON *root)
{
    char path[CAP_PAIRING_PATH_SIZE];
    char *json = NULL;
    FILE *file = NULL;
    esp_err_t err;

    err = cap_pairing_build_storage_path(channel, path, sizeof(path));
    if (err != ESP_OK) {
        return err;
    }

    json = cJSON_PrintUnformatted(root);
    if (!json) {
        return ESP_ERR_NO_MEM;
    }

    file = fopen(path, "wb");
    if (!file) {
        free(json);
        return ESP_FAIL;
    }
    if (fputs(json, file) < 0) {
        fclose(file);
        free(json);
        return ESP_FAIL;
    }
    fclose(file);
    free(json);
    return ESP_OK;
}

static bool cap_pairing_is_expired(cJSON *request, int64_t now_ms)
{
    cJSON *created_at = cJSON_GetObjectItemCaseSensitive(request, "created_at");
    if (!cJSON_IsNumber(created_at)) {
        return true;
    }

    int64_t created_ms = (int64_t)created_at->valuedouble;
    return (now_ms - created_ms) > CAP_PAIRING_PENDING_TTL_MS;
}

static esp_err_t cap_pairing_prune_store_locked(cap_pairing_channel_t channel, cJSON *root, int *pruned_count)
{
    cJSON *requests = cJSON_GetObjectItemCaseSensitive(root, "requests");
    cJSON *request, *tmp;
    int64_t now_ms;
    int count = 0;

    if (!cJSON_IsArray(requests)) {
        return ESP_ERR_INVALID_STATE;
    }

    now_ms = cap_pairing_get_time_ms();
    
    cJSON_ArrayForEachSafe(request, tmp, requests) {
        if (cap_pairing_is_expired(request, now_ms)) {
            cJSON_DeleteItemFromArray(requests, request->index);
            count++;
        }
    }

    if (pruned_count) {
        *pruned_count = count;
    }

    return ESP_OK;
}

esp_err_t cap_pairing_init(const char *pairing_root_dir)
{
    int written;
    SemaphoreHandle_t mutex = s_pairing.mutex;

    if (!pairing_root_dir || !pairing_root_dir[0]) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(&s_pairing, 0, sizeof(s_pairing));
    s_pairing.mutex = mutex;
    
    strlcpy(s_pairing.pairing_root_dir, pairing_root_dir, sizeof(s_pairing.pairing_root_dir));

    if (!s_pairing.mutex) {
        s_pairing.mutex = xSemaphoreCreateRecursiveMutex();
    }
    if (!s_pairing.mutex) {
        return ESP_ERR_NO_MEM;
    }

    if (cap_pairing_ensure_dir(s_pairing.pairing_root_dir) != ESP_OK) {
        vSemaphoreDelete(s_pairing.mutex);
        s_pairing.mutex = NULL;
        return ESP_FAIL;
    }

    s_pairing.initialized = true;
    ESP_LOGI(TAG, "Pairing manager initialized, root dir: %s", s_pairing.pairing_root_dir);
    return ESP_OK;
}

esp_err_t cap_pairing_deinit(void)
{
    if (s_pairing.mutex) {
        vSemaphoreDelete(s_pairing.mutex);
        s_pairing.mutex = NULL;
    }
    s_pairing.initialized = false;
    ESP_LOGI(TAG, "Pairing manager deinitialized");
    return ESP_OK;
}

esp_err_t cap_pairing_generate_code(cap_pairing_channel_t channel, char *out_code, size_t out_code_size)
{
    size_t alphabet_len = strlen(CAP_PAIRING_CODE_ALPHABET);
    size_t i;
    uint32_t random_val;

    (void)channel;

    if (!out_code || out_code_size <= CAP_PAIRING_CODE_LENGTH) {
        return ESP_ERR_INVALID_ARG;
    }

    // Generate random code using system time and a counter
    for (i = 0; i < CAP_PAIRING_CODE_LENGTH; i++) {
        random_val = (uint32_t)(cap_pairing_get_time_ms() ^ i);
        out_code[i] = CAP_PAIRING_CODE_ALPHABET[random_val % alphabet_len];
    }
    out_code[CAP_PAIRING_CODE_LENGTH] = '\0';

    return ESP_OK;
}

bool cap_pairing_validate_code_format(const char *code)
{
    size_t i;

    if (!code || strlen(code) != CAP_PAIRING_CODE_LENGTH) {
        return false;
    }

    for (i = 0; i < CAP_PAIRING_CODE_LENGTH; i++) {
        char c = code[i];
        if (strchr(CAP_PAIRING_CODE_ALPHABET, c) == NULL) {
            return false;
        }
    }
    return true;
}

esp_err_t cap_pairing_create_request(cap_pairing_channel_t channel, const char *code, 
                                     const char *meta, char *out_request_id, size_t out_request_id_size)
{
    cJSON *root = NULL;
    cJSON *requests = NULL;
    cJSON *new_request = NULL;
    char request_id[CAP_PAIRING_ID_SIZE];
    int64_t now_ms;
    int active_count = 0;
    int pruned = 0;
    esp_err_t err;

    if (!s_pairing.initialized || !s_pairing.mutex) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!code || !out_request_id || out_request_id_size < CAP_PAIRING_ID_SIZE) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!cap_pairing_validate_code_format(code)) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTakeRecursive(s_pairing.mutex, portMAX_DELAY);

    // Read existing store
    err = cap_pairing_read_store(channel, &root);
    if (err != ESP_OK) {
        xSemaphoreGiveRecursive(s_pairing.mutex);
        return err;
    }

    requests = cJSON_GetObjectItemCaseSensitive(root, "requests");
    if (!cJSON_IsArray(requests)) {
        cJSON_Delete(root);
        xSemaphoreGiveRecursive(s_pairing.mutex);
        return ESP_ERR_INVALID_STATE;
    }

    // Prune expired first
    cap_pairing_prune_store_locked(channel, root, &pruned);
    if (pruned > 0) {
        ESP_LOGD(TAG, "Pruned %d expired requests", pruned);
    }

    // Count active requests
    cJSON *req;
    cJSON_ArrayForEach(req, requests) {
        cJSON *active = cJSON_GetObjectItemCaseSensitive(req, "active");
        if (cJSON_IsTrue(active)) {
            active_count++;
        }
    }

    if (active_count >= CAP_PAIRING_PENDING_MAX) {
        cJSON_Delete(root);
        xSemaphoreGiveRecursive(s_pairing.mutex);
        ESP_LOGW(TAG, "Maximum pending requests reached (%d)", CAP_PAIRING_PENDING_MAX);
        return ESP_ERR_NO_MEM;
    }

    // Generate new request ID
    cap_pairing_generate_random_id(request_id, sizeof(request_id));

    // Create new request object
    new_request = cJSON_CreateObject();
    if (!new_request) {
        cJSON_Delete(root);
        xSemaphoreGiveRecursive(s_pairing.mutex);
        return ESP_ERR_NO_MEM;
    }

    now_ms = cap_pairing_get_time_ms();
    cJSON_AddStringToObject(new_request, "id", request_id);
    cJSON_AddStringToObject(new_request, "code", code);
    cJSON_AddNumberToObject(new_request, "created_at", (double)now_ms);
    cJSON_AddNumberToObject(new_request, "last_seen_at", (double)now_ms);
    cJSON_AddBoolToObject(new_request, "active", true);
    if (meta && meta[0]) {
        cJSON_AddStringToObject(new_request, "meta", meta);
    } else {
        cJSON_AddNullToObject(new_request, "meta");
    }

    cJSON_AddItemToArray(requests, new_request);

    // Write back to store
    err = cap_pairing_write_store(channel, root);
    cJSON_Delete(root);

    if (err == ESP_OK) {
        strncpy(out_request_id, request_id, out_request_id_size - 1);
        out_request_id[out_request_id_size - 1] = '\0';
        ESP_LOGI(TAG, "Created pairing request %s with code %s", request_id, code);
    }

    xSemaphoreGiveRecursive(s_pairing.mutex);
    return err;
}

esp_err_t cap_pairing_get_request(const char *request_id, cap_pairing_request_t *out_request)
{
    cJSON *root = NULL;
    cJSON *requests = NULL;
    cJSON *req;
    esp_err_t err = ESP_ERR_NOT_FOUND;

    if (!s_pairing.initialized || !request_id || !out_request) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTakeRecursive(s_pairing.mutex, portMAX_DELAY);

    err = cap_pairing_read_store(CAP_PAIRING_CHANNEL_NONE, &root);
    if (err != ESP_OK) {
        xSemaphoreGiveRecursive(s_pairing.mutex);
        return err;
    }

    requests = cJSON_GetObjectItemCaseSensitive(root, "requests");
    if (!cJSON_IsArray(requests)) {
        cJSON_Delete(root);
        xSemaphoreGiveRecursive(s_pairing.mutex);
        return ESP_ERR_INVALID_STATE;
    }

    // Search in all channels
    for (int ch = CAP_PAIRING_CHANNEL_DISCORD; ch < CAP_PAIRING_CHANNEL_MAX; ch++) {
        char path[CAP_PAIRING_PATH_SIZE];
        FILE *file = NULL;
        long size;
        char *text = NULL;
        
        err = cap_pairing_build_storage_path((cap_pairing_channel_t)ch, path, sizeof(path));
        if (err != ESP_OK) {
            continue;
        }

        file = fopen(path, "rb");
        if (!file) {
            continue;
        }

        if (fseek(file, 0, SEEK_END) != 0) {
            fclose(file);
            continue;
        }
        size = ftell(file);
        if (size < 0 || size > 1024 * 1024) {
            fclose(file);
            continue;
        }
        fseek(file, 0, SEEK_SET);

        text = calloc(1, (size_t)size + 1);
        if (!text) {
            fclose(file);
            continue;
        }
        fread(text, 1, (size_t)size, file);
        fclose(file);
        text[size] = '\0';

        cJSON *ch_root = cJSON_Parse(text);
        free(text);
        
        if (cJSON_IsObject(ch_root)) {
            cJSON *ch_requests = cJSON_GetObjectItemCaseSensitive(ch_root, "requests");
            if (cJSON_IsArray(ch_requests)) {
                cJSON_ArrayForEach(req, ch_requests) {
                    cJSON *id_item = cJSON_GetObjectItemCaseSensitive(req, "id");
                    if (cJSON_IsString(id_item) && strcmp(id_item->valuestring, request_id) == 0) {
                        // Found it
                        cJSON *code_item = cJSON_GetObjectItemCaseSensitive(req, "code");
                        cJSON *created_item = cJSON_GetObjectItemCaseSensitive(req, "created_at");
                        cJSON *last_seen_item = cJSON_GetObjectItemCaseSensitive(req, "last_seen_at");
                        cJSON *meta_item = cJSON_GetObjectItemCaseSensitive(req, "meta");
                        cJSON *active_item = cJSON_GetObjectItemCaseSensitive(req, "active");

                        strncpy(out_request->id, request_id, sizeof(out_request->id) - 1);
                        out_request->id[sizeof(out_request->id) - 1] = '\0';
                        
                        if (cJSON_IsString(code_item)) {
                            strncpy(out_request->code, code_item->valuestring, sizeof(out_request->code) - 1);
                        }
                        out_request->created_at = cJSON_IsNumber(created_item) ? (int64_t)created_item->valuedouble : 0;
                        out_request->last_seen_at = cJSON_IsNumber(last_seen_item) ? (int64_t)last_seen_item->valuedouble : 0;
                        
                        if (cJSON_IsString(meta_item)) {
                            strncpy(out_request->meta, meta_item->valuestring, sizeof(out_request->meta) - 1);
                        } else {
                            out_request->meta[0] = '\0';
                        }
                        
                        out_request->active = cJSON_IsTrue(active_item);

                        cJSON_Delete(ch_root);
                        xSemaphoreGiveRecursive(s_pairing.mutex);
                        return ESP_OK;
                    }
                }
            }
        }
        cJSON_Delete(ch_root);
    }

    cJSON_Delete(root);
    xSemaphoreGiveRecursive(s_pairing.mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t cap_pairing_touch_request(const char *request_id)
{
    cJSON *root = NULL;
    cJSON *requests = NULL;
    cJSON *req;
    bool found = false;
    int64_t now_ms;
    esp_err_t err = ESP_ERR_NOT_FOUND;

    if (!s_pairing.initialized || !request_id) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTakeRecursive(s_pairing.mutex, portMAX_DELAY);

    // Try all channels
    for (int ch = CAP_PAIRING_CHANNEL_DISCORD; ch < CAP_PAIRING_CHANNEL_MAX; ch++) {
        err = cap_pairing_read_store((cap_pairing_channel_t)ch, &root);
        if (err != ESP_OK) {
            continue;
        }

        requests = cJSON_GetObjectItemCaseSensitive(root, "requests");
        if (!cJSON_IsArray(requests)) {
            cJSON_Delete(root);
            continue;
        }

        cJSON_ArrayForEach(req, requests) {
            cJSON *id_item = cJSON_GetObjectItemCaseSensitive(req, "id");
            if (cJSON_IsString(id_item) && strcmp(id_item->valuestring, request_id) == 0) {
                now_ms = cap_pairing_get_time_ms();
                cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(req, "last_seen_at"), (double)now_ms);
                
                err = cap_pairing_write_store((cap_pairing_channel_t)ch, root);
                found = true;
                break;
            }
        }

        cJSON_Delete(root);
        if (found) {
            break;
        }
    }

    xSemaphoreGiveRecursive(s_pairing.mutex);
    return found ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t cap_pairing_accept_request(const char *request_id, const char *user_id)
{
    cJSON *root = NULL;
    cJSON *requests = NULL;
    cJSON *req;
    bool found = false;
    esp_err_t err = ESP_ERR_NOT_FOUND;

    (void)user_id;  // Could be stored in meta for audit

    if (!s_pairing.initialized || !request_id) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTakeRecursive(s_pairing.mutex, portMAX_DELAY);

    for (int ch = CAP_PAIRING_CHANNEL_DISCORD; ch < CAP_PAIRING_CHANNEL_MAX; ch++) {
        err = cap_pairing_read_store((cap_pairing_channel_t)ch, &root);
        if (err != ESP_OK) {
            continue;
        }

        requests = cJSON_GetObjectItemCaseSensitive(root, "requests");
        if (!cJSON_IsArray(requests)) {
            cJSON_Delete(root);
            continue;
        }

        cJSON_ArrayForEach(req, requests) {
            cJSON *id_item = cJSON_GetObjectItemCaseSensitive(req, "id");
            if (cJSON_IsString(id_item) && strcmp(id_item->valuestring, request_id) == 0) {
                cJSON_ReplaceItemInObject(req, "active", cJSON_CreateFalse());
                
                err = cap_pairing_write_store((cap_pairing_channel_t)ch, root);
                found = true;
                break;
            }
        }

        cJSON_Delete(root);
        if (found) {
            break;
        }
    }

    xSemaphoreGiveRecursive(s_pairing.mutex);
    
    if (found) {
        ESP_LOGI(TAG, "Accepted pairing request %s", request_id);
    }
    
    return found ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t cap_pairing_reject_request(const char *request_id)
{
    cJSON *root = NULL;
    cJSON *requests = NULL;
    cJSON *req;
    bool found = false;
    esp_err_t err = ESP_ERR_NOT_FOUND;

    if (!s_pairing.initialized || !request_id) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTakeRecursive(s_pairing.mutex, portMAX_DELAY);

    for (int ch = CAP_PAIRING_CHANNEL_DISCORD; ch < CAP_PAIRING_CHANNEL_MAX; ch++) {
        err = cap_pairing_read_store((cap_pairing_channel_t)ch, &root);
        if (err != ESP_OK) {
            continue;
        }

        requests = cJSON_GetObjectItemCaseSensitive(root, "requests");
        if (!cJSON_IsArray(requests)) {
            cJSON_Delete(root);
            continue;
        }

        cJSON_ArrayForEach(req, requests) {
            cJSON *id_item = cJSON_GetObjectItemCaseSensitive(req, "id");
            if (cJSON_IsString(id_item) && strcmp(id_item->valuestring, request_id) == 0) {
                cJSON_DeleteItemFromArray(requests, req->index);
                
                err = cap_pairing_write_store((cap_pairing_channel_t)ch, root);
                found = true;
                break;
            }
        }

        cJSON_Delete(root);
        if (found) {
            break;
        }
    }

    xSemaphoreGiveRecursive(s_pairing.mutex);
    
    if (found) {
        ESP_LOGI(TAG, "Rejected pairing request %s", request_id);
    }
    
    return found ? ESP_OK : ESP_ERR_NOT_FOUND;
}

size_t cap_pairing_prune_expired(void)
{
    int total_pruned = 0;
    int pruned;

    if (!s_pairing.initialized || !s_pairing.mutex) {
        return 0;
    }

    xSemaphoreTakeRecursive(s_pairing.mutex, portMAX_DELAY);

    for (int ch = CAP_PAIRING_CHANNEL_DISCORD; ch < CAP_PAIRING_CHANNEL_MAX; ch++) {
        cJSON *root = NULL;
        esp_err_t err;

        err = cap_pairing_read_store((cap_pairing_channel_t)ch, &root);
        if (err != ESP_OK) {
            continue;
        }

        cap_pairing_prune_store_locked((cap_pairing_channel_t)ch, root, &pruned);
        
        if (pruned > 0) {
            cap_pairing_write_store((cap_pairing_channel_t)ch, root);
            total_pruned += pruned;
        }

        cJSON_Delete(root);
    }

    xSemaphoreGiveRecursive(s_pairing.mutex);
    
    if (total_pruned > 0) {
        ESP_LOGI(TAG, "Pruned %d expired pairing requests", total_pruned);
    }
    
    return (size_t)total_pruned;
}

esp_err_t cap_pairing_list_active(cap_pairing_channel_t channel, 
                                  cap_pairing_request_t **out_requests,
                                  size_t max_requests, size_t *out_count)
{
    cJSON *root = NULL;
    cJSON *requests = NULL;
    cJSON *req;
    size_t count = 0;
    esp_err_t err;

    (void)out_requests;  // Simplified for embedded - just return count
    (void)max_requests;

    if (!s_pairing.initialized || !out_count) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTakeRecursive(s_pairing.mutex, portMAX_DELAY);

    err = cap_pairing_read_store(channel, &root);
    if (err != ESP_OK) {
        xSemaphoreGiveRecursive(s_pairing.mutex);
        return err;
    }

    requests = cJSON_GetObjectItemCaseSensitive(root, "requests");
    if (!cJSON_IsArray(requests)) {
        cJSON_Delete(root);
        xSemaphoreGiveRecursive(s_pairing.mutex);
        return ESP_ERR_INVALID_STATE;
    }

    int64_t now_ms = cap_pairing_get_time_ms();
    cJSON_ArrayForEach(req, requests) {
        cJSON *active = cJSON_GetObjectItemCaseSensitive(req, "active");
        if (cJSON_IsTrue(active) && !cap_pairing_is_expired(req, now_ms)) {
            count++;
        }
    }

    cJSON_Delete(root);
    *out_count = count;
    
    xSemaphoreGiveRecursive(s_pairing.mutex);
    return ESP_OK;
}

esp_err_t cap_pairing_get_storage_path(cap_pairing_channel_t channel, char *buf, size_t buf_size)
{
    if (!s_pairing.initialized || !buf || buf_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return cap_pairing_build_storage_path(channel, buf, buf_size);
}

// Capability call implementations
static esp_err_t cap_pairing_generate_execute(const char *input_json,
                                              const claw_cap_call_context_t *ctx,
                                              char *output,
                                              size_t output_size)
{
    char code[16];
    char request_id[CAP_PAIRING_ID_SIZE];
    esp_err_t err;

    (void)input_json;
    (void)ctx;

    if (!s_pairing.initialized) {
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"pairing not initialized\"}");
        }
        return ESP_ERR_INVALID_STATE;
    }

    err = cap_pairing_generate_code(CAP_PAIRING_CHANNEL_DISCORD, code, sizeof(code));
    if (err != ESP_OK) {
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"%s\"}", esp_err_to_name(err));
        }
        return err;
    }

    err = cap_pairing_create_request(CAP_PAIRING_CHANNEL_DISCORD, code, NULL, request_id, sizeof(request_id));
    if (err != ESP_OK) {
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"%s\"}", esp_err_to_name(err));
        }
        return err;
    }

    if (output && output_size > 0) {
        snprintf(output, output_size, "{\"ok\":true,\"code\":\"%s\",\"request_id\":\"%s\"}", code, request_id);
    }

    ESP_LOGI(TAG, "Generated pairing code: %s", code);
    return ESP_OK;
}

static esp_err_t cap_pairing_accept_execute(const char *input_json,
                                            const claw_cap_call_context_t *ctx,
                                            char *output,
                                            size_t output_size)
{
    cJSON *input = NULL;
    cJSON *request_id_item = NULL;
    const char *request_id;
    esp_err_t err;

    (void)ctx;

    if (!s_pairing.initialized) {
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"pairing not initialized\"}");
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

    request_id_item = cJSON_GetObjectItemCaseSensitive(input, "request_id");
    if (!cJSON_IsString(request_id_item)) {
        cJSON_Delete(input);
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"request_id required\"}");
        }
        return ESP_ERR_INVALID_ARG;
    }

    request_id = request_id_item->valuestring;
    err = cap_pairing_accept_request(request_id, NULL);
    cJSON_Delete(input);

    if (err != ESP_OK) {
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"%s\"}", esp_err_to_name(err));
        }
        return err;
    }

    if (output && output_size > 0) {
        snprintf(output, output_size, "{\"ok\":true,\"message\":\"Pairing accepted\"}");
    }

    return ESP_OK;
}

static esp_err_t cap_pairing_status_execute(const char *input_json,
                                            const claw_cap_call_context_t *ctx,
                                            char *output,
                                            size_t output_size)
{
    size_t active_count = 0;
    esp_err_t err;

    (void)input_json;
    (void)ctx;

    if (!s_pairing.initialized) {
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"pairing not initialized\"}");
        }
        return ESP_ERR_INVALID_STATE;
    }

    err = cap_pairing_list_active(CAP_PAIRING_CHANNEL_DISCORD, NULL, 0, &active_count);
    if (err != ESP_OK) {
        if (output && output_size > 0) {
            snprintf(output, output_size, "{\"ok\":false,\"error\":\"%s\"}", esp_err_to_name(err));
        }
        return err;
    }

    if (output && output_size > 0) {
        snprintf(output, output_size, "{\"ok\":true,\"active_requests\":%zu}", active_count);
    }

    return ESP_OK;
}

static const claw_cap_descriptor_t s_pairing_caps[] = {
    {
        .id = "generate_pairing_code",
        .name = "generate_pairing_code",
        .family = "pairing",
        .description = "Generate a new pairing code for device authentication",
        .kind = CLAW_CAP_KIND_CALLABLE,
        .cap_flags = CLAW_CAP_FLAG_RESTRICTED,
        .input_schema_json = "{\"type\":\"object\",\"properties\":{}}",
        .execute = cap_pairing_generate_execute,
    },
    {
        .id = "accept_pairing",
        .name = "accept_pairing",
        .family = "pairing",
        .description = "Accept a pending pairing request",
        .kind = CLAW_CAP_KIND_CALLABLE,
        .cap_flags = CLAW_CAP_FLAG_RESTRICTED,
        .input_schema_json = "{\"type\":\"object\",\"required\":[\"request_id\"],\"properties\":{\"request_id\":{\"type\":\"string\"}}}",
        .execute = cap_pairing_accept_execute,
    },
    {
        .id = "pairing_status",
        .name = "pairing_status",
        .family = "pairing",
        .description = "Get current pairing status",
        .kind = CLAW_CAP_KIND_CALLABLE,
        .cap_flags = 0,
        .input_schema_json = "{\"type\":\"object\",\"properties\":{}}",
        .execute = cap_pairing_status_execute,
    },
};

static const claw_cap_group_t s_pairing_group = {
    .group_id = "cap_pairing",
    .plugin_name = "cap_pairing",
    .version = "1.0.0",
    .descriptors = s_pairing_caps,
    .descriptor_count = sizeof(s_pairing_caps) / sizeof(s_pairing_caps[0]),
};

esp_err_t cap_pairing_register_group(void)
{
    return claw_cap_register_group(&s_pairing_group);
}
