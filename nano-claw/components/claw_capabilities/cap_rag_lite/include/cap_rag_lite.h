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
