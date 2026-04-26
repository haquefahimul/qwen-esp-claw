#!/usr/bin/env python3
import os

components = {
    "cap_hooks": {
        "desc": "Event Hook System",
        "source": "OpenClaw",
        "features": ["8 event types", "5 priority levels", "Chainable hooks", "Abort capability"]
    },
    "cap_heartbeat": {
        "desc": "Heartbeat Monitor", 
        "source": "OpenClaw",
        "features": ["Periodic check-ins", "1s-24h intervals", "Auto-reconnect", "Channel health"]
    },
    "cap_pairing": {
        "desc": "Device Pairing System",
        "source": "OpenClaw", 
        "features": ["8-char codes", "5 channels", "1h TTL", "JSON storage"]
    },
    "cap_secrets": {
        "desc": "Encrypted Secrets Vault",
        "source": "OpenClaw",
        "features": ["AES-256-GCM", "5 secret types", "Rotation", "Secure wipe"]
    },
    "cap_ratelimit": {
        "desc": "Rate Limiter",
        "source": "zclaw/OpenClaw",
        "features": ["Sliding window", "Burst allowance", "Per-user limits", "Global limits"]
    },
    "cap_react": {
        "desc": "ReAct Agent Engine",
        "source": "femtoclaw",
        "features": ["Reason+Act pattern", "Tool registry", "Pattern matching", "Multi-step"]
    },
    "cap_context": {
        "desc": "Context Engine",
        "source": "OpenClaw/femtoclaw",
        "features": ["Sliding window", "Token counting", "Memory management", "Session history"]
    },
    "cap_vector": {
        "desc": "Vector Store Lite",
        "source": "mimiclaw/zeroclaw",
        "features": ["In-PSRAM index", "Cosine similarity", "Top-K search", "Embedding cache"]
    },
    "cap_eventbus": {
        "desc": "Event Bus Pub-Sub",
        "source": "zeroclaw",
        "features": ["Decoupled messaging", "Topic-based", "Async delivery", "Queue management"]
    },
    "cap_watchdog": {
        "desc": "Watchdog & Safe Mode",
        "source": "zclaw",
        "features": ["Task monitoring", "Stack analysis", "Auto-reboot", "Crash recovery"]
    },
    "cap_prompt": {
        "desc": "Prompt Template Engine",
        "source": "zeroclaw/OpenClaw",
        "features": ["Mustache syntax", "Variable substitution", "Conditional blocks", "Loop support"]
    }
}

base_path = "/workspace/neon-claw/components/neon_capabilities"

for comp, info in components.items():
    header_path = f"{base_path}/{comp}/include/{comp}.h"
    source_path = f"{base_path}/{comp}/src/{comp}.c"
    
    # Generate header file
    header_content = f'''/**
 * @file {comp}.h
 * @brief {info["desc"]} for Neon-Claw
 * 
 * Features: {", ".join(info["features"])}
 * Source: Ported from {info["source"]}
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Configuration constants
#define {comp.upper().replace("_", "_")}_MAX_ITEMS 32
#define {comp.upper().replace("_", "_")}_BUFFER_SIZE 1024

/**
 * @brief Initialize {info["desc"]}
 * @return ESP_OK on success
 */
esp_err_t {comp.replace("_", "_")}_init(void);

/**
 * @brief Deinitialize {info["desc"]}
 * @return ESP_OK on success
 */
esp_err_t {comp.replace("_", "_")}_deinit(void);

/**
 * @brief Main processing function
 * @param data Input data pointer
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t {comp.replace("_", "_")}_process(const void* data, size_t len);

/**
 * @brief Get component status
 * @return true if active
 */
bool {comp.replace("_", "_")}_is_active(void);

/**
 * @brief Get statistics
 * @param count Pointer to store item count
 * @return ESP_OK on success
 */
esp_err_t {comp.replace("_", "_")}_get_stats(int* count);

#endif // _{comp.upper()}_H
'''
    
    # Generate source file stub
    source_content = f'''/**
 * @file {comp}.c
 * @brief {info["desc"]} Implementation
 * 
 * Ported from {info["source"]}
 * Features: {", ".join(info["features"])}
 */

#include "{comp}.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <stdlib.h>

static const char* TAG = "{comp}";
static bool initialized = false;
static int item_count = 0;

esp_err_t {comp.replace("_", "_")}_init(void) {{
    if (initialized) {{
        return ESP_ERR_INVALID_STATE;
    }}
    
    ESP_LOGI(TAG, "Initializing {info["desc"]}...");
    // TODO: Implement initialization logic
    // - Allocate memory
    // - Create tasks/timers
    // - Load persisted state
    
    initialized = true;
    ESP_LOGI(TAG, "{info["desc"]} initialized successfully");
    return ESP_OK;
}}

esp_err_t {comp.replace("_", "_")}_deinit(void) {{
    if (!initialized) {{
        return ESP_ERR_INVALID_STATE;
    }}
    
    ESP_LOGI(TAG, "Deinitializing {info["desc"]}...");
    // TODO: Implement cleanup
    // - Free memory
    // - Stop tasks/timers
    // - Save state
    
    initialized = false;
    item_count = 0;
    ESP_LOGI(TAG, "{info["desc"]} deinitialized");
    return ESP_OK;
}}

esp_err_t {comp.replace("_", "_")}_process(const void* data, size_t len) {{
    if (!initialized || !data) {{
        return ESP_ERR_INVALID_ARG;
    }}
    
    // TODO: Implement main processing logic
    // Based on features: {", ".join(info["features"])}
    
    ESP_LOGD(TAG, "Processing %d bytes", len);
    item_count++;
    return ESP_OK;
}}

bool {comp.replace("_", "_")}_is_active(void) {{
    return initialized;
}}

esp_err_t {comp.replace("_", "_")}_get_stats(int* count) {{
    if (!count) {{
        return ESP_ERR_INVALID_ARG;
    }}
    *count = item_count;
    return ESP_OK;
}}
'''
    
    with open(header_path, 'w') as f:
        f.write(header_content)
    
    with open(source_path, 'w') as f:
        f.write(source_content)
    
    print(f"✓ Generated {comp}")

print("\n✅ All component files generated!")
print("Next step: Implement specific logic in each .c file")
