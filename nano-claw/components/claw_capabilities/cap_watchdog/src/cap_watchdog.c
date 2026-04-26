#include "cap_watchdog.h"
#include "esp_log.h"

static const char *TAG = "cap_watchdog";

esp_err_t cap_watchdog_init(void) {
    ESP_LOGI(TAG, "cap_watchdog initialized (stub implementation)");
    return ESP_OK;
}

esp_err_t cap_watchdog_deinit(void) {
    ESP_LOGI(TAG, "cap_watchdog deinitialized");
    return ESP_OK;
}
