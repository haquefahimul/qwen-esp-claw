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
