/*
 * Neon-Claw Core - Main Agent Loop
 * 
 * Combines ReAct pattern from femtoclaw, event bus from zeroclaw,
 * and session management from mimiclaw into a unified agent loop.
 * 
 * By maruf009sultan
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

// Core includes
#include "neon_claw_core.h"
#include "app_claw.h"
#include "tools_registry.h"

// Capability includes
#include "cap_cron.h"
#include "cap_hooks.h"
#include "cap_heartbeat.h"
#include "cap_pairing.h"
#include "cap_secrets_vault.h"
#include "cap_ratelimit.h"
#include "cap_react_agent.h"
#include "cap_context_engine.h"
#include "cap_vector_store.h"
#include "cap_watchdog.h"
#include "cap_power.h"

static const char *TAG = "NEON_CLAW";

// Global state
static neon_claw_state_t g_state = {
    .initialized = false,
    .paired = false,
    .current_mode = NEON_MODE_IDLE,
    .session_count = 0,
    .uptime_start = 0
};

// Event queue
static QueueHandle_t s_event_queue = NULL;

esp_err_t neon_claw_init(void)
{
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Initializing Neon-Claw Framework...");
    
    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize core capabilities
    ESP_LOGI(TAG, "Initializing capabilities...");
    
    ret = cap_cron_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init cron: %s", esp_err_to_name(ret));
    }
    
    ret = cap_hooks_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init hooks: %s", esp_err_to_name(ret));
    }
    
    ret = cap_heartbeat_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init heartbeat: %s", esp_err_to_name(ret));
    }
    
    ret = cap_pairing_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init pairing: %s", esp_err_to_name(ret));
    }
    
    ret = cap_secrets_vault_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init secrets vault: %s", esp_err_to_name(ret));
    }
    
    ret = cap_ratelimit_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init rate limiter: %s", esp_err_to_name(ret));
    }
    
    ret = cap_react_agent_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init ReAct agent: %s", esp_err_to_name(ret));
    }
    
    ret = cap_context_engine_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init context engine: %s", esp_err_to_name(ret));
    }
    
    ret = cap_vector_store_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init vector store: %s", esp_err_to_name(ret));
    }
    
    ret = cap_watchdog_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init watchdog: %s", esp_err_to_name(ret));
    }
    
    ret = cap_power_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init power manager: %s", esp_err_to_name(ret));
    }
    
    // Create event queue
    s_event_queue = xQueueCreate(NEON_EVENT_QUEUE_SIZE, sizeof(neon_event_t));
    if (s_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize tools registry
    ret = tools_registry_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init tools registry: %s", esp_err_to_name(ret));
    }
    
    // Initialize base CLAW
    ret = app_claw_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init CLAW: %s", esp_err_to_name(ret));
    }
    
    g_state.initialized = true;
    g_state.uptime_start = esp_timer_get_time() / 1000000ULL;
    
    ESP_LOGI(TAG, "Neon-Claw initialized successfully!");
    ESP_LOGI(TAG, "Device ready for pairing");
    
    return ESP_OK;
}

esp_err_t neon_claw_start(void)
{
    if (!g_state.initialized) {
        ESP_LOGE(TAG, "Neon-Claw not initialized!");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Starting Neon-Claw agent loop...");
    
    // Start capability tasks
    cap_cron_start();
    cap_heartbeat_start();
    cap_watchdog_start();
    
    // Start main agent task
    xTaskCreatePinnedToCore(
        neon_claw_agent_task,
        "neon_agent",
        NEON_AGENT_STACK_SIZE,
        NULL,
        NEON_AGENT_PRIORITY,
        NULL,
        PRO_CPU_NUM
    );
    
    g_state.current_mode = NEON_MODE_RUNNING;
    
    return ESP_OK;
}

void neon_claw_agent_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Agent task started on core %d", xPortGetCoreID());
    
    neon_event_t event;
    
    while (1) {
        // Wait for events with timeout
        if (xQueueReceive(s_event_queue, &event, pdMS_TO_TICKS(1000)) == pdTRUE) {
            ESP_LOGD(TAG, "Processing event: %d", event.type);
            
            // Run pre-event hooks
            cap_hooks_trigger(HOOK_PRE_EVENT, &event);
            
            // Process event based on type
            switch (event.type) {
                case NEON_EVENT_USER_MESSAGE:
                    handle_user_message(&event);
                    break;
                    
                case NEON_EVENT_CRON_TRIGGER:
                    handle_cron_trigger(&event);
                    break;
                    
                case NEON_EVENT_HEARTBEAT:
                    handle_heartbeat(&event);
                    break;
                    
                case NEON_EVENT_PAIRING_REQUEST:
                    handle_pairing_request(&event);
                    break;
                    
                case NEON_EVENT_TOOL_RESULT:
                    handle_tool_result(&event);
                    break;
                    
                default:
                    ESP_LOGW(TAG, "Unknown event type: %d", event.type);
                    break;
            }
            
            // Run post-event hooks
            cap_hooks_trigger(HOOK_POST_EVENT, &event);
        }
        
        // Periodic tasks (run every loop iteration if no events)
        cap_react_agent_loop();
        
        // Feed watchdog
        cap_watchdog_feed();
    }
}

static void handle_user_message(neon_event_t *event)
{
    ESP_LOGI(TAG, "Handling user message from session: %s", event->data.session_id);
    
    // Check rate limit
    if (!cap_ratelimit_check(event->data.session_id)) {
        ESP_LOGW(TAG, "Rate limit exceeded for session: %s", event->data.session_id);
        send_error_response(event->data.session_id, "Rate limit exceeded. Please try again later.");
        return;
    }
    
    // Add to context
    cap_context_engine_append(event->data.session_id, "user", event->data.content);
    
    // Build context for LLM
    char context[4096];
    cap_context_engine_build(event->data.session_id, context, sizeof(context));
    
    // Send to LLM via CLAW
    // This would integrate with the existing app_claw.c LLM calls
    ESP_LOGI(TAG, "Sending to LLM: %d bytes", strlen(context));
    
    // Response will come back asynchronously and trigger NEON_EVENT_LLM_RESPONSE
}

static void handle_cron_trigger(neon_event_t *event)
{
    ESP_LOGI(TAG, "Cron trigger: %s", event->data.action);
    
    // Execute scheduled action
    // This could trigger sensor readings, reports, etc.
    neon_event_t new_event = {
        .type = NEON_EVENT_USER_MESSAGE,
        .data = {
            .session_id = "cron_system",
            .content = event->data.action
        }
    };
    
    xQueueSend(s_event_queue, &new_event, portMAX_DELAY);
}

static void handle_heartbeat(neon_event_t *event)
{
    ESP_LOGD(TAG, "Heartbeat check");
    
    // Send periodic status to paired channels
    if (g_state.paired) {
        // Send uptime, memory status, etc.
        uint32_t uptime = (esp_timer_get_time() / 1000000ULL) - g_state.uptime_start;
        ESP_LOGI(TAG, "Heartbeat - Uptime: %lu seconds", uptime);
    }
}

static void handle_pairing_request(neon_event_t *event)
{
    ESP_LOGI(TAG, "Pairing request received");
    
    // Generate pairing code
    char code[9];
    cap_pairing_generate(code, sizeof(code));
    
    ESP_LOGI(TAG, "Generated pairing code: %s", code);
    
    // Send code to user (via serial, display, or configured channel)
    printf("\n========================================\n");
    printf("  NEON-CLAW PAIRING CODE: %s\n", code);
    printf("  Valid for 60 minutes\n");
    printf("  Use /pair %s in Telegram/Discord\n", code);
    printf("========================================\n\n");
}

static void handle_tool_result(neon_event_t *event)
{
    ESP_LOGI(TAG, "Tool result received");
    
    // Add tool result to context
    cap_context_engine_append(event->data.session_id, "tool", event->data.content);
    
    // Continue agent loop
    cap_react_agent_continue(event->data.session_id);
}

esp_err_t neon_claw_send_event(neon_event_type_t type, const char *session_id, const char *content)
{
    if (!g_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    neon_event_t event = {
        .type = type,
        .data = {
            .session_id = "",
            .content = ""
        }
    };
    
    if (session_id) {
        strncpy(event.data.session_id, session_id, sizeof(event.data.session_id) - 1);
    }
    
    if (content) {
        strncpy(event.data.content, content, sizeof(event.data.content) - 1);
    }
    
    if (xQueueSend(s_event_queue, &event, pdMS_TO_TICKS(1000)) != pdPASS) {
        ESP_LOGE(TAG, "Failed to send event to queue");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

neon_claw_state_t* neon_claw_get_state(void)
{
    return &g_state;
}

void send_error_response(const char *session_id, const char *error_msg)
{
    // Send error response to user
    // This would integrate with the messaging channel
    ESP_LOGE(TAG, "Error for session %s: %s", session_id, error_msg);
}
