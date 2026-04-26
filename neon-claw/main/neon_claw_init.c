/**
 * @file neon_claw_init.c
 * @brief Initialize all Neon-Claw components
 * 
 * This file initializes all capabilities ported from:
 * - OpenClaw (Hooks, Cron, Heartbeat, Pairing, Secrets)
 * - zclaw (Rate Limit, Watchdog)
 * - femtoclaw (ReAct Agent, Context)
 * - mimiclaw/zeroclaw (Vector Store, Event Bus, Prompt Templates)
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

// Include all capability headers
#include "cap_cron.h"
#include "cap_hooks.h"
#include "cap_heartbeat.h"
#include "cap_pairing.h"
#include "cap_secrets.h"
#include "cap_ratelimit.h"
#include "cap_react.h"
#include "cap_context.h"
#include "cap_vector.h"
#include "cap_eventbus.h"
#include "cap_watchdog.h"
#include "cap_prompt.h"

static const char* TAG = "neon_init";

esp_err_t neon_claw_init_all(void) {
    esp_err_t ret;
    
    ESP_LOGI(TAG, "=== NEON-CLAW INITIALIZATION ===");
    ESP_LOGI(TAG, "Initializing %d enhanced capabilities...", 12);
    
    // Initialize NVS first (required by many components)
    ESP_LOGI(TAG, "[1/13] Initializing NVS...");
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "✓ NVS initialized");
    
    // Initialize all capabilities
    ESP_LOGI(TAG, "[2/13] Initializing Cron Scheduler...");
    ret = cron_init();
    if (ret != ESP_OK) ESP_LOGE(TAG, "Failed to init cron: %s", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "✓ Cron Scheduler ready");
    
    ESP_LOGI(TAG, "[3/13] Initializing Hook System...");
    ret = cap_hooks_init();
    if (ret != ESP_OK) ESP_LOGE(TAG, "Failed to init hooks: %s", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "✓ Hook System ready");
    
    ESP_LOGI(TAG, "[4/13] Initializing Heartbeat Monitor...");
    ret = cap_heartbeat_init();
    if (ret != ESP_OK) ESP_LOGE(TAG, "Failed to init heartbeat: %s", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "✓ Heartbeat Monitor ready");
    
    ESP_LOGI(TAG, "[5/13] Initializing Device Pairing...");
    ret = cap_pairing_init();
    if (ret != ESP_OK) ESP_LOGE(TAG, "Failed to init pairing: %s", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "✓ Device Pairing ready");
    
    ESP_LOGI(TAG, "[6/13] Initializing Secrets Vault...");
    ret = cap_secrets_init();
    if (ret != ESP_OK) ESP_LOGE(TAG, "Failed to init secrets: %s", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "✓ Secrets Vault ready");
    
    ESP_LOGI(TAG, "[7/13] Initializing Rate Limiter...");
    ret = cap_ratelimit_init();
    if (ret != ESP_OK) ESP_LOGE(TAG, "Failed to init ratelimit: %s", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "✓ Rate Limiter ready");
    
    ESP_LOGI(TAG, "[8/13] Initializing ReAct Agent...");
    ret = cap_react_init();
    if (ret != ESP_OK) ESP_LOGE(TAG, "Failed to init react: %s", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "✓ ReAct Agent ready");
    
    ESP_LOGI(TAG, "[9/13] Initializing Context Engine...");
    ret = cap_context_init();
    if (ret != ESP_OK) ESP_LOGE(TAG, "Failed to init context: %s", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "✓ Context Engine ready");
    
    ESP_LOGI(TAG, "[10/13] Initializing Vector Store...");
    ret = cap_vector_init();
    if (ret != ESP_OK) ESP_LOGE(TAG, "Failed to init vector: %s", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "✓ Vector Store ready");
    
    ESP_LOGI(TAG, "[11/13] Initializing Event Bus...");
    ret = cap_eventbus_init();
    if (ret != ESP_OK) ESP_LOGE(TAG, "Failed to init eventbus: %s", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "✓ Event Bus ready");
    
    ESP_LOGI(TAG, "[12/13] Initializing Watchdog...");
    ret = cap_watchdog_init();
    if (ret != ESP_OK) ESP_LOGE(TAG, "Failed to init watchdog: %s", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "✓ Watchdog ready");
    
    ESP_LOGI(TAG, "[13/13] Initializing Prompt Templates...");
    ret = cap_prompt_init();
    if (ret != ESP_OK) ESP_LOGE(TAG, "Failed to init prompt: %s", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "✓ Prompt Templates ready");
    
    ESP_LOGI(TAG, "=== ALL COMPONENTS INITIALIZED ===");
    return ESP_OK;
}

esp_err_t neon_claw_deinit_all(void) {
    ESP_LOGI(TAG, "Deinitializing all Neon-Claw components...");
    
    cap_prompt_deinit();
    cap_watchdog_deinit();
    cap_eventbus_deinit();
    cap_vector_deinit();
    cap_context_deinit();
    cap_react_deinit();
    cap_ratelimit_deinit();
    cap_secrets_deinit();
    cap_pairing_deinit();
    cap_heartbeat_deinit();
    cap_hooks_deinit();
    cron_deinit();
    
    nvs_flash_deinit();
    
    ESP_LOGI(TAG, "All components deinitialized");
    return ESP_OK;
}
