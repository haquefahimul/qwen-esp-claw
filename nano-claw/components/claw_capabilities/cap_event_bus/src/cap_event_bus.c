#include "cap_event_bus.h"
#include "esp_log.h"

static const char *TAG = "cap_event_bus";

esp_err_t cap_event_bus_init(void) {
    ESP_LOGI(TAG, "cap_event_bus initialized (stub implementation)");
    return ESP_OK;
}

esp_err_t cap_event_bus_deinit(void) {
    ESP_LOGI(TAG, "cap_event_bus deinitialized");
    return ESP_OK;
}
