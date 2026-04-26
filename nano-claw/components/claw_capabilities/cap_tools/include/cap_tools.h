/**
 * @file cap_tools.h
 * @brief Dynamic Tool Registration & Execution Engine
 */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "esp_err.h"
#include <stddef.h>

#define MAX_TOOLS 32
#define MAX_TOOL_NAME 32
#define MAX_TOOL_DESC 128

typedef esp_err_t (*tool_func_t)(const char* json_args, char* output, size_t output_len);

typedef struct {
    char name[MAX_TOOL_NAME];
    char description[MAX_TOOL_DESC];
    tool_func_t callback;
    int usage_count;
    TickType_t last_used;
    bool enabled;
} tool_info_t;

typedef struct {
    tool_info_t tools[MAX_TOOLS];
    size_t count;
    SemaphoreHandle_t mutex;
} tool_engine_t;

tool_engine_t* tool_engine_init(void);
void tool_engine_deinit(tool_engine_t* engine);
esp_err_t tool_register(tool_engine_t* engine, const char* name, const char* desc, tool_func_t cb);
esp_err_t tool_execute(tool_engine_t* engine, const char* name, const char* args, char* out, size_t len);
esp_err_t tool_unregister(tool_engine_t* engine, const char* name);
tool_info_t* tool_list(tool_engine_t* engine, size_t* count);
esp_err_t tool_enable(tool_engine_t* engine, const char* name);
esp_err_t tool_disable(tool_engine_t* engine, const char* name);

// Built-in tools
esp_err_t tool_builtin_time(const char* args, char* out, size_t len);
esp_err_t tool_builtin_calculate(const char* args, char* out, size_t len);

#ifdef __cplusplus
}
#endif
