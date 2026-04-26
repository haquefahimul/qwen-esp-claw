/**
 * @file cap_storage.c
 * @brief Implementation of Type-Safe NVS Wrapper
 */

#include "cap_storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char* TAG = "cap_storage";
static nvs_handle_t s_storage_handle = 0;
static bool s_initialized = false;

esp_err_t storage_init(void) {
    if (s_initialized) {
        return ESP_OK;
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_open("nano_claw", NVS_READWRITE, &s_storage_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Storage initialized");
    return ESP_OK;
}

void storage_deinit(void) {
    if (s_initialized) {
        nvs_close(s_storage_handle);
        s_initialized = false;
        ESP_LOGI(TAG, "Storage deinitialized");
    }
}

esp_err_t storage_save_int(const char* key, int32_t value) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    esp_err_t err = nvs_set_i32(s_storage_handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(s_storage_handle);
    }
    return err;
}

esp_err_t storage_load_int(const char* key, int32_t* out_value) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    return nvs_get_i32(s_storage_handle, key, out_value);
}

esp_err_t storage_save_str(const char* key, const char* value) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    esp_err_t err = nvs_set_str(s_storage_handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(s_storage_handle);
    }
    return err;
}

esp_err_t storage_load_str(const char* key, char* out_buffer, size_t buffer_size) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    size_t len = buffer_size;
    esp_err_t err = nvs_get_str(s_storage_handle, key, out_buffer, &len);
    return err;
}

esp_err_t storage_save_blob(const char* key, const void* data, size_t data_len) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    esp_err_t err = nvs_set_blob(s_storage_handle, key, data, data_len);
    if (err == ESP_OK) {
        err = nvs_commit(s_storage_handle);
    }
    return err;
}

esp_err_t storage_load_blob(const char* key, void* out_buffer, size_t buffer_size, size_t* out_len) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    size_t len = buffer_size;
    esp_err_t err = nvs_get_blob(s_storage_handle, key, out_buffer, &len);
    if (err == ESP_OK && out_len) {
        *out_len = len;
    }
    return err;
}

esp_err_t storage_delete(const char* key) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    esp_err_t err = nvs_erase_key(s_storage_handle, key);
    if (err == ESP_OK) {
        err = nvs_commit(s_storage_handle);
    }
    return err;
}

esp_err_t storage_erase_all(void) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    esp_err_t err = nvs_erase_all(s_storage_handle);
    if (err == ESP_OK) {
        err = nvs_commit(s_storage_handle);
    }
    return err;
}
