/* Nano-Claw ReAct Agent Implementation - Copyright 2024 */
#include "cap_react.h"
#include <string.h>
#include "esp_log.h"

static const char *TAG = "cap_react";
#define MAX_TOOLS 16

typedef struct {
    tool_t tools[MAX_TOOLS];
    bool initialized;
} react_ctx_t;

static react_ctx_t s_react_ctx;

esp_err_t react_init(void) {
    if (s_react_ctx.initialized) return ESP_OK;
    memset(&s_react_ctx, 0, sizeof(react_ctx_t));
    s_react_ctx.initialized = true;
    ESP_LOGI(TAG, "ReAct agent initialized");
    return ESP_OK;
}

esp_err_t react_register_tool(const char *name, const char *desc, tool_func_t func) {
    if (!s_react_ctx.initialized || !name || !func) return ESP_ERR_INVALID_ARG;
    for (int i = 0; i < MAX_TOOLS; i++) {
        if (s_react_ctx.tools[i].name[0] == '\0') {
            strncpy(s_react_ctx.tools[i].name, name, TOOL_NAME_MAX - 1);
            strncpy(s_react_ctx.tools[i].description, desc, TOOL_DESC_MAX - 1);
            s_react_ctx.tools[i].func = func;
            ESP_LOGI(TAG, "Tool '%s' registered", name);
            return ESP_OK;
        }
    }
    return ESP_ERR_NO_MEM;
}

esp_err_t react_execute(const char *thought, char *action, char *result, size_t max_len) {
    if (!s_react_ctx.initialized || !thought) return ESP_ERR_INVALID_ARG;
    
    // Simple pattern matching for action selection
    for (int i = 0; i < MAX_TOOLS; i++) {
        if (s_react_ctx.tools[i].name[0] != '\0' && 
            strstr(thought, s_react_ctx.tools[i].name)) {
            if (action) strncpy(action, s_react_ctx.tools[i].name, max_len);
            if (result && s_react_ctx.tools[i].func) {
                return s_react_ctx.tools[i].func(thought, result, max_len);
            }
            return ESP_OK;
        }
    }
    
    if (action) strncpy(action, "none", max_len);
    if (result) snprintf(result, max_len, "No matching tool found");
    return ESP_ERR_NOT_FOUND;
}

uint32_t react_get_tool_count(void) {
    uint32_t count = 0;
    for (int i = 0; i < MAX_TOOLS; i++) {
        if (s_react_ctx.tools[i].name[0] != '\0') count++;
    }
    return count;
}
