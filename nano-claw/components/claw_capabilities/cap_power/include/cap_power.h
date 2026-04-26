#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "esp_err.h"
#include "driver/gpio.h"
typedef enum { POWER_ACTIVE, POWER_LIGHT_SLEEP, POWER_DEEP_SLEEP } power_mode_t;
typedef struct { bool enable_battery_monitor; gpio_num_t battery_pin; uint32_t sleep_ms; } power_config_t;
esp_err_t power_init(power_config_t* cfg);
esp_err_t power_set_mode(power_mode_t mode);
esp_err_t power_enter_light_sleep(uint32_t ms);
esp_err_t power_enter_deep_sleep(uint32_t ms, gpio_num_t wake_pin);
int power_get_battery_level(void);
esp_err_t power_enable_wifi_ps(void);
#ifdef __cplusplus
}
#endif
