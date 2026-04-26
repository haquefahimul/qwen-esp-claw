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
