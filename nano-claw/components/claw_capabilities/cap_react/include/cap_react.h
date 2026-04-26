/* Nano-Claw ReAct Agent - Copyright 2024 */
#ifndef CAP_REACT_H
#define CAP_REACT_H
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TOOL_NAME_MAX 32
#define TOOL_DESC_MAX 128

typedef esp_err_t (*tool_func_t)(const char *args, char *result, size_t max_len);

typedef struct {
    char name[TOOL_NAME_MAX];
    char description[TOOL_DESC_MAX];
    tool_func_t func;
} tool_t;

esp_err_t react_init(void);
esp_err_t react_register_tool(const char *name, const char *desc, tool_func_t func);
esp_err_t react_execute(const char *thought, char *action, char *result, size_t max_len);
uint32_t react_get_tool_count(void);

#ifdef __cplusplus
}
#endif
#endif
