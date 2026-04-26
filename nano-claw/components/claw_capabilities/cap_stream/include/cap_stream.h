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
