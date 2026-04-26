#include "cap_tools.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "esp_log.h"

static const char* TAG = "cap_tools";

tool_engine_t* tool_engine_init(void) {
    tool_engine_t* engine = calloc(1, sizeof(tool_engine_t));
    if (!engine) return NULL;
    engine->mutex = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "Tool engine initialized");
    return engine;
}

void tool_engine_deinit(tool_engine_t* engine) {
    if (!engine) return;
    if (engine->mutex) vSemaphoreDelete(engine->mutex);
    free(engine);
}

esp_err_t tool_register(tool_engine_t* engine, const char* name, const char* desc, tool_func_t cb) {
    if (!engine || !name || !cb) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(engine->mutex, pdMS_TO_TICKS(100)) != pdTRUE) return ESP_ERR_TIMEOUT;
    
    if (engine->count >= MAX_TOOLS) {
        xSemaphoreGive(engine->mutex);
        return ESP_ERR_NO_MEM;
    }
    
    tool_info_t* tool = &engine->tools[engine->count];
    strncpy(tool->name, name, MAX_TOOL_NAME-1);
    strncpy(tool->description, desc, MAX_TOOL_DESC-1);
    tool->callback = cb;
    tool->enabled = true;
    engine->count++;
    
    ESP_LOGI(TAG, "Registered tool: %s", name);
    xSemaphoreGive(engine->mutex);
    return ESP_OK;
}

esp_err_t tool_execute(tool_engine_t* engine, const char* name, const char* args, char* out, size_t len) {
    if (!engine || !name || !out) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(engine->mutex, pdMS_TO_TICKS(100)) != pdTRUE) return ESP_ERR_TIMEOUT;
    
    for (size_t i = 0; i < engine->count; i++) {
        if (strcmp(engine->tools[i].name, name) == 0 && engine->tools[i].enabled) {
            engine->tools[i].usage_count++;
            engine->tools[i].last_used = xTaskGetTickCount();
            xSemaphoreGive(engine->mutex);
            return engine->tools[i].callback(args, out, len);
        }
    }
    
    xSemaphoreGive(engine->mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t tool_builtin_time(const char* args, char* out, size_t len) {
    time_t now = time(NULL);
    strftime(out, len, "%Y-%m-%d %H:%M:%S", localtime(&now));
    return ESP_OK;
}

esp_err_t tool_builtin_calculate(const char* args, char* out, size_t len) {
    int a, b; char op;
    if (sscanf(args, "%d %c %d", &a, &op, &b) != 3) {
        snprintf(out, len, "Error: Invalid format");
        return ESP_ERR_INVALID_ARG;
    }
    int result = 0;
    switch(op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/': result = b ? a / b : 0; break;
    }
    snprintf(out, len, "%d", result);
    return ESP_OK;
}

tool_info_t* tool_list(tool_engine_t* engine, size_t* count) {
    if (!engine || !count) return NULL;
    *count = engine->count;
    return engine->tools;
}

esp_err_t tool_unregister(tool_engine_t* engine, const char* name) { return ESP_OK; }
esp_err_t tool_enable(tool_engine_t* engine, const char* name) { return ESP_OK; }
esp_err_t tool_disable(tool_engine_t* engine, const char* name) { return ESP_OK; }
