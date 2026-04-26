#include "cap_ratelimit.h"
#include "esp_log.h"

static const char *TAG = "cap_ratelimit";

esp_err_t cap_ratelimit_init(void) {
    ESP_LOGI(TAG, "cap_ratelimit initialized (stub implementation)");
    return ESP_OK;
}

esp_err_t cap_ratelimit_deinit(void) {
    ESP_LOGI(TAG, "cap_ratelimit deinitialized");
    return ESP_OK;
}
