#include "cap_context_engine.h"
#include "esp_log.h"

static const char *TAG = "cap_context_engine";

esp_err_t cap_context_engine_init(void) {
    ESP_LOGI(TAG, "cap_context_engine initialized (stub implementation)");
    return ESP_OK;
}

esp_err_t cap_context_engine_deinit(void) {
    ESP_LOGI(TAG, "cap_context_engine deinitialized");
    return ESP_OK;
}
