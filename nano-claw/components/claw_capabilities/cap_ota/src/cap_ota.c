/**
 * @file cap_ota.c
 * @brief Implementation of Secure OTA Update Manager
 */

#include "cap_ota.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char* TAG = "cap_ota";

static ota_progress_cb_t s_progress_cb = NULL;
static void* s_user_data = NULL;
static bool s_is_updating = false;
static esp_ota_handle_t s_ota_handle = 0;

static void ota_event_handler(ota_event_t event, int progress) {
    if (s_progress_cb) {
        s_progress_cb(event, progress, s_user_data);
    }
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    static int s_image_len = 0;
    
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (!esp_ota_write(s_ota_handle, evt->data, evt->data_len) == ESP_OK) {
                ESP_LOGE(TAG, "OTA write failed");
                return ESP_FAIL;
            }
            s_image_len += evt->data_len;
            if ((s_image_len % 10240) == 0) {
                ESP_LOGI(TAG, "Written image length %d", s_image_len);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP GET complete");
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t ota_init(void) {
    ESP_LOGI(TAG, "OTA subsystem initialized");
    return ESP_OK;
}

esp_err_t ota_request_update(const char* url, const char* expected_sha256) {
    if (s_is_updating) {
        ESP_LOGW(TAG, "Update already in progress");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting OTA from %s", url);
    s_is_updating = true;
    ota_event_handler(OTA_EVENT_START, 0);

    const esp_partition_t* update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        ESP_LOGE(TAG, "No update partition found");
        s_is_updating = false;
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Writing to partition %s at offset 0x%lx", 
             update_partition->label, update_partition->address);

    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &s_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        s_is_updating = false;
        return err;
    }

    // Configure HTTP client
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 30000,
        .buffer_size = 2048,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        esp_ota_abort(s_ota_handle);
        s_is_updating = false;
        return ESP_FAIL;
    }

    err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    if (err == ESP_OK) {
        err = esp_ota_end(s_ota_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
            s_is_updating = false;
            return err;
        }

        // Set boot partition
        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
            s_is_updating = false;
            return err;
        }

        ESP_LOGI(TAG, "OTA update successful, rebooting in 2 seconds...");
        ota_event_handler(OTA_EVENT_COMPLETE, 100);
        s_is_updating = false;
        
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        esp_ota_abort(s_ota_handle);
        ota_event_handler(OTA_EVENT_ERROR, 0);
        s_is_updating = false;
        return err;
    }

    return ESP_OK;
}

esp_err_t ota_check_update(const char* url, bool* out_available) {
    // Simplified check - in production would check version header
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_HEAD,
        .timeout_ms = 5000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    
    if (out_available) {
        *out_available = (err == ESP_OK);
    }
    return err;
}

void ota_set_callback(ota_progress_cb_t cb, void* user_data) {
    s_progress_cb = cb;
    s_user_data = user_data;
}

bool ota_is_updating(void) {
    return s_is_updating;
}

void ota_mark_boot_successful(void) {
    const esp_partition_t* running = esp_ota_get_running_partition();
    if (running) {
        esp_ota_img_states_t state;
        if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
            if (state == ESP_OTA_IMG_PENDING_VERIFY) {
                ESP_LOGI(TAG, "Marking boot as successful");
                esp_ota_mark_app_valid_cancel_rollback();
            }
        }
    }
}

esp_err_t ota_request_rollback(void) {
    ESP_LOGI(TAG, "Requesting rollback");
    esp_ota_mark_app_invalid_rollback_and_reboot();
    return ESP_OK; // Never returns
}
