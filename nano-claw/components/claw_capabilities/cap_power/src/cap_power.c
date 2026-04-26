#include "cap_power.h"
#include "esp_sleep.h"
#include "driver/adc.h"
#include "esp_log.h"
static const char* TAG = "cap_power";
static power_config_t g_cfg;
esp_err_t power_init(power_config_t* cfg) {
    g_cfg = cfg ? *cfg : (power_config_t){0};
    if (g_cfg.enable_battery_monitor) {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(g_cfg.battery_pin, ADC_ATTEN_DB_11);
    }
    ESP_LOGI(TAG, "Power manager initialized");
    return ESP_OK;
}
esp_err_t power_set_mode(power_mode_t mode) {
    switch(mode) {
        case POWER_LIGHT_SLEEP: return power_enter_light_sleep(0);
        case POWER_DEEP_SLEEP: return power_enter_deep_sleep(0, GPIO_NUM_MAX);
        default: return ESP_OK;
    }
}
esp_err_t power_enter_light_sleep(uint32_t ms) {
    ESP_LOGI(TAG, "Entering light sleep for %ums", ms);
    esp_light_sleep_start();
    return ESP_OK;
}
esp_err_t power_enter_deep_sleep(uint32_t ms, gpio_num_t pin) {
    ESP_LOGI(TAG, "Entering deep sleep");
    if (pin != GPIO_NUM_MAX) esp_sleep_enable_ext0_wakeup(pin, 0);
    if (ms > 0) esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_deep_sleep_start();
    return ESP_OK;
}
int power_get_battery_level(void) {
    if (!g_cfg.enable_battery_monitor) return -1;
    int raw = adc1_get_raw(g_cfg.battery_pin);
    return (raw * 100) / 4095;
}
esp_err_t power_enable_wifi_ps(void) { return ESP_OK; }
