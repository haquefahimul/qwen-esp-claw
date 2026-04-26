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
