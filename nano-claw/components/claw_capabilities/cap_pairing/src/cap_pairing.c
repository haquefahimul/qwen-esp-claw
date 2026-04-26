#include "cap_pairing.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

static const char *TAG = "pairing";
static pairing_request_t pending_requests[MAX_PENDING_PAIRINGS * 5];
static const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

static void generate_random_code(char *code, size_t len) {
    for (size_t i = 0; i < len - 1; i++) {
        if (i == 4) code[i] = '-';
        else code[i] = alphabet[rand() % (sizeof(alphabet) - 1)];
    }
    code[len - 1] = '\0';
}

esp_err_t pairing_init(void) {
    memset(pending_requests, 0, sizeof(pending_requests));
    srand(time(NULL));
    ESP_LOGI(TAG, "Pairing system initialized");
    return ESP_OK;
}

esp_err_t pairing_generate_code(pairing_channel_t channel, char *code, size_t len) {
    if (len < PAIRING_CODE_LEN) return ESP_ERR_INVALID_SIZE;
    
    int slot = -1;
    for (int i = 0; i < MAX_PENDING_PAIRINGS * 5; i++) {
        if (!pending_requests[i].used && !pending_requests[i].code[0]) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        ESP_LOGW(TAG, "No available pairing slots");
        return ESP_ERR_NO_MEM;
    }
    
    generate_random_code(code, len);
    strncpy(pending_requests[slot].code, code, PAIRING_CODE_LEN - 1);
    pending_requests[slot].channel = channel;
    pending_requests[slot].expiry_timestamp = time(NULL) + 3600; // 1 hour
    pending_requests[slot].used = false;
    
    ESP_LOGI(TAG, "Generated pairing code %s for channel %d", code, channel);
    return ESP_OK;
}

esp_err_t pairing_accept_code(const char *code, const char *device_id) {
    for (int i = 0; i < MAX_PENDING_PAIRINGS * 5; i++) {
        if (strcmp(pending_requests[i].code, code) == 0) {
            if (pending_requests[i].used) {
                ESP_LOGW(TAG, "Code %s already used", code);
                return ESP_ERR_INVALID_STATE;
            }
            if (time(NULL) > pending_requests[i].expiry_timestamp) {
                ESP_LOGW(TAG, "Code %s expired", code);
                pending_requests[i].code[0] = '\0';
                return ESP_ERR_TIMEOUT;
            }
            
            pending_requests[i].used = true;
            ESP_LOGI(TAG, "Device %s paired with code %s", device_id, code);
            return ESP_OK;
        }
    }
    
    ESP_LOGW(TAG, "Invalid pairing code: %s", code);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t pairing_get_status(const char *code, bool *is_valid, bool *is_used) {
    for (int i = 0; i < MAX_PENDING_PAIRINGS * 5; i++) {
        if (strcmp(pending_requests[i].code, code) == 0) {
            *is_valid = (time(NULL) <= pending_requests[i].expiry_timestamp);
            *is_used = pending_requests[i].used;
            return ESP_OK;
        }
    }
    *is_valid = false;
    *is_used = false;
    return ESP_ERR_NOT_FOUND;
}

esp_err_t pairing_cleanup_expired(void) {
    time_t now = time(NULL);
    int cleaned = 0;
    for (int i = 0; i < MAX_PENDING_PAIRINGS * 5; i++) {
        if (pending_requests[i].code[0] && 
            (pending_requests[i].used || now > pending_requests[i].expiry_timestamp)) {
            pending_requests[i].code[0] = '\0';
            cleaned++;
        }
    }
    ESP_LOGD(TAG, "Cleaned up %d expired pairing codes", cleaned);
    return ESP_OK;
}

int pairing_get_pending_count(pairing_channel_t channel) {
    int count = 0;
    time_t now = time(NULL);
    for (int i = 0; i < MAX_PENDING_PAIRINGS * 5; i++) {
        if (pending_requests[i].code[0] && 
            pending_requests[i].channel == channel &&
            !pending_requests[i].used &&
            now <= pending_requests[i].expiry_timestamp) {
            count++;
        }
    }
    return count;
}
