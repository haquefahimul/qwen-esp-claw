#include "cap_heartbeat.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "heartbeat";
static heartbeat_config_t g_config;
static esp_timer_handle_t heartbeat_timer;

static void heartbeat_timer_cb(void *arg) {
    if (g_config.enabled && g_config.callback) {
        g_config.callback(g_config.arg);
        ESP_LOGD(TAG, "Heartbeat sent");
    }
}

esp_err_t heartbeat_init(heartbeat_config_t *config) {
    if (!config) return ESP_ERR_INVALID_ARG;
    memcpy(&g_config, config, sizeof(heartbeat_config_t));
    
    esp_timer_create_args_t timer_args = {
        .callback = heartbeat_timer_cb,
        .name = "heartbeat_timer"
    };
    
    esp_err_t ret = esp_timer_create(&timer_args, &heartbeat_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create heartbeat timer");
        return ret;
    }
    
    ESP_LOGI(TAG, "Heartbeat initialized with interval %d ms", g_config.interval_ms);
    return ESP_OK;
}

esp_err_t heartbeat_start(void) {
    g_config.enabled = true;
    esp_err_t ret = esp_timer_start_periodic(heartbeat_timer, g_config.interval_ms * 1000);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Heartbeat started");
    }
    return ret;
}

esp_err_t heartbeat_stop(void) {
    g_config.enabled = false;
    esp_err_t ret = esp_timer_stop(heartbeat_timer);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Heartbeat stopped");
    }
    return ret;
}

esp_err_t heartbeat_set_interval(uint32_t interval_ms) {
    g_config.interval_ms = interval_ms;
    if (g_config.enabled) {
        esp_timer_stop(heartbeat_timer);
        return esp_timer_start_periodic(heartbeat_timer, interval_ms * 1000);
    }
    return ESP_OK;
}

esp_err_t heartbeat_send_now(void) {
    if (g_config.callback) {
        g_config.callback(g_config.arg);
        return ESP_OK;
    }
    return ESP_ERR_INVALID_STATE;
}
