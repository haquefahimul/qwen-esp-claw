#!/bin/bash
echo "🚀 Generating Phase 2 Components for Nano-Claw..."

# Component 2: cap_tools
cat > components/claw_capabilities/cap_tools/include/cap_tools.h << 'EOF'
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
EOF

cat > components/claw_capabilities/cap_tools/src/cap_tools.c << 'EOF'
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
EOF

cat > components/claw_capabilities/cap_tools/CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)
idf_component_register(SRCS "src/cap_tools.c" INCLUDE_DIRS "include" REQUIRES freertos json)
EOF

# Component 3: cap_stream
cat > components/claw_capabilities/cap_stream/include/cap_stream.h << 'EOF'
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "esp_err.h"
#include <stddef.h>

typedef enum { STREAM_EVENT_DELTA, STREAM_EVENT_COMPLETE, STREAM_EVENT_ERROR, STREAM_EVENT_DONE } stream_event_type_t;
typedef void (*stream_callback_t)(stream_event_type_t type, const char* data, size_t len, void* user_data);

typedef struct {
    char* buffer;
    size_t size;
    size_t capacity;
    stream_callback_t callback;
    void* user_data;
    bool in_event;
} stream_parser_t;

stream_parser_t* stream_parser_create(stream_callback_t cb, void* user_data);
void stream_parser_destroy(stream_parser_t* parser);
esp_err_t stream_parser_feed(stream_parser_t* parser, const char* data, size_t len);
esp_err_t stream_parser_finalize(stream_parser_t* parser);
#ifdef __cplusplus
}
#endif
EOF

cat > components/claw_capabilities/cap_stream/src/cap_stream.c << 'EOF'
#include "cap_stream.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
static const char* TAG = "cap_stream";

stream_parser_t* stream_parser_create(stream_callback_t cb, void* user_data) {
    stream_parser_t* p = calloc(1, sizeof(stream_parser_t));
    if (!p) return NULL;
    p->capacity = 4096;
    p->buffer = malloc(p->capacity);
    p->callback = cb;
    p->user_data = user_data;
    return p;
}

void stream_parser_destroy(stream_parser_t* p) {
    if (!p) return;
    free(p->buffer);
    free(p);
}

esp_err_t stream_parser_feed(stream_parser_t* p, const char* data, size_t len) {
    if (!p || !data) return ESP_ERR_INVALID_ARG;
    if (p->size + len >= p->capacity) {
        p->capacity *= 2;
        p->buffer = realloc(p->buffer, p->capacity);
    }
    memcpy(p->buffer + p->size, data, len);
    p->size += len;
    
    // Simple SSE parsing
    char* line_start = p->buffer;
    char* newline;
    while ((newline = memchr(line_start, '\n', p->size - (line_start - p->buffer)))) {
        if (strncmp(line_start, "data: ", 6) == 0) {
            size_t data_len = newline - line_start - 6;
            if (p->callback) p->callback(STREAM_EVENT_DELTA, line_start + 6, data_len, p->user_data);
        }
        line_start = newline + 1;
    }
    return ESP_OK;
}

esp_err_t stream_parser_finalize(stream_parser_t* p) {
    if (!p) return ESP_ERR_INVALID_ARG;
    if (p->callback) p->callback(STREAM_EVENT_DONE, NULL, 0, p->user_data);
    p->size = 0;
    return ESP_OK;
}
EOF

cat > components/claw_capabilities/cap_stream/CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)
idf_component_register(SRCS "src/cap_stream.c" INCLUDE_DIRS "include")
EOF

# Component 4: cap_fsm
cat > components/claw_capabilities/cap_fsm/include/cap_fsm.h << 'EOF'
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "esp_err.h"
#define MAX_STATES 16
#define MAX_TRANSITIONS 64
typedef enum { STATE_IDLE, STATE_LISTENING, STATE_THINKING, STATE_ACTING, STATE_RESPONDING, STATE_ERROR } fsm_state_t;
typedef void (*state_entry_t)(void*);
typedef void (*state_exit_t)(void*);
typedef bool (*guard_func_t)(void*);
typedef struct { fsm_state_t from, to; const char* event; guard_func_t guard; } transition_t;
typedef struct { const char* name; state_entry_t entry; state_exit_t exit; } state_def_t;
typedef struct { state_def_t states[MAX_STATES]; transition_t transitions[MAX_TRANSITIONS]; size_t state_count, trans_count; fsm_state_t current; void* user_data; } fsm_t;
fsm_t* fsm_create(const char* name);
esp_err_t fsm_add_state(fsm_t* f, const char* name, state_entry_t e, state_exit_t x);
esp_err_t fsm_add_transition(fsm_t* f, const char* from, const char* to, const char* event, guard_func_t g);
esp_err_t fsm_trigger(fsm_t* f, const char* event);
const char* fsm_get_state(fsm_t* f);
#ifdef __cplusplus
}
#endif
EOF

cat > components/claw_capabilities/cap_fsm/src/cap_fsm.c << 'EOF'
#include "cap_fsm.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
static const char* TAG = "cap_fsm";
static const char* state_names[] = {"IDLE","LISTENING","THINKING","ACTING","RESPONDING","ERROR"};
fsm_t* fsm_create(const char* name) { fsm_t* f = calloc(1,sizeof(fsm_t)); f->current=STATE_IDLE; return f; }
esp_err_t fsm_add_state(fsm_t* f, const char* name, state_entry_t e, state_exit_t x) {
    if (f->state_count >= MAX_STATES) return ESP_ERR_NO_MEM;
    f->states[f->state_count++] = (state_def_t){name, e, x};
    return ESP_OK;
}
esp_err_t fsm_add_transition(fsm_t* f, const char* from, const char* to, const char* event, guard_func_t g) {
    if (f->trans_count >= MAX_TRANSITIONS) return ESP_ERR_NO_MEM;
    f->transitions[f->trans_count++] = (transition_t){atoi(from), atoi(to), event, g};
    return ESP_OK;
}
esp_err_t fsm_trigger(fsm_t* f, const char* event) {
    for (size_t i=0; i<f->trans_count; i++) {
        if (strcmp(f->transitions[i].event, event)==0 && f->transitions[i].from==f->current) {
            if (f->states[f->current].exit) f->states[f->current].exit(f->user_data);
            f->current = f->transitions[i].to;
            if (f->states[f->current].entry) f->states[f->current].entry(f->user_data);
            ESP_LOGI(TAG, "Transition: %s -> %s via %s", state_names[f->transitions[i].from], state_names[f->current], event);
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}
const char* fsm_get_state(fsm_t* f) { return state_names[f->current]; }
EOF

cat > components/claw_capabilities/cap_fsm/CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)
idf_component_register(SRCS "src/cap_fsm.c" INCLUDE_DIRS "include")
EOF

# Component 5: cap_rag_lite
cat > components/claw_capabilities/cap_rag_lite/include/cap_rag_lite.h << 'EOF'
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "esp_err.h"
#define MAX_DOCS 512
#define MAX_DIM 384
typedef struct { char id[32]; float vector[MAX_DIM]; char* content; } doc_t;
typedef struct { doc_t docs[MAX_DOCS]; size_t count; float idf[MAX_DIM]; } rag_engine_t;
typedef struct { char doc_id[32]; float score; } rag_result_t;
rag_engine_t* rag_create(size_t max);
esp_err_t rag_add_doc(rag_engine_t* r, const char* id, const char* content);
esp_err_t rag_index(rag_engine_t* r);
rag_result_t* rag_query(rag_engine_t* r, const char* query, size_t k, size_t* result_count);
esp_err_t rag_save(rag_engine_t* r, const char* path);
esp_err_t rag_load(rag_engine_t* r, const char* path);
#ifdef __cplusplus
}
#endif
EOF

cat > components/claw_capabilities/cap_rag_lite/src/cap_rag_lite.c << 'EOF'
#include "cap_rag_lite.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
static const char* TAG = "cap_rag_lite";
rag_engine_t* rag_create(size_t max) { return calloc(1, sizeof(rag_engine_t)); }
static void simple_tokenize(const char* text, float* vec, size_t dim) {
    memset(vec, 0, dim*sizeof(float));
    for (size_t i=0; i<strlen(text) && i<dim; i++) vec[i] = (float)(unsigned char)text[i] / 255.0f;
}
esp_err_t rag_add_doc(rag_engine_t* r, const char* id, const char* content) {
    if (r->count >= MAX_DOCS) return ESP_ERR_NO_MEM;
    doc_t* d = &r->docs[r->count];
    strncpy(d->id, id, 31);
    d->content = strdup(content);
    simple_tokenize(content, d->vector, MAX_DIM);
    r->count++;
    return ESP_OK;
}
esp_err_t rag_index(rag_engine_t* r) { /* TF-IDF computation stub */ return ESP_OK; }
static float cosine_sim(float* a, float* b, size_t dim) {
    float dot=0, na=0, nb=0;
    for (size_t i=0; i<dim; i++) { dot+=a[i]*b[i]; na+=a[i]*a[i]; nb+=b[i]*b[i]; }
    return dot / (sqrtf(na)*sqrtf(nb) + 1e-9);
}
rag_result_t* rag_query(rag_engine_t* r, const char* query, size_t k, size_t* rc) {
    float qvec[MAX_DIM]; simple_tokenize(query, qvec, MAX_DIM);
    rag_result_t* results = calloc(k, sizeof(rag_result_t));
    for (size_t i=0; i<r->count && i<k; i++) {
        strncpy(results[i].doc_id, r->docs[i].id, 31);
        results[i].score = cosine_sim(qvec, r->docs[i].vector, MAX_DIM);
    }
    *rc = k;
    return results;
}
esp_err_t rag_save(rag_engine_t* r, const char* path) { return ESP_OK; }
esp_err_t rag_load(rag_engine_t* r, const char* path) { return ESP_OK; }
EOF

cat > components/claw_capabilities/cap_rag_lite/CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)
idf_component_register(SRCS "src/cap_rag_lite.c" INCLUDE_DIRS "include")
EOF

# Component 6: cap_power
cat > components/claw_capabilities/cap_power/include/cap_power.h << 'EOF'
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
EOF

cat > components/claw_capabilities/cap_power/src/cap_power.c << 'EOF'
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
EOF

cat > components/claw_capabilities/cap_power/CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)
idf_component_register(SRCS "src/cap_power.c" INCLUDE_DIRS "include" REQUIRES driver esp_pm)
EOF

echo "✅ All Phase 2 components generated!"
