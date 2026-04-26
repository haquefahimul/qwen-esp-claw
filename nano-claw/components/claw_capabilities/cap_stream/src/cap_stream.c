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
