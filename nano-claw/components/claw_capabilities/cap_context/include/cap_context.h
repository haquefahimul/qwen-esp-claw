/**
 * @file cap_context.h
 * @brief Context Window Management for Nano-Claw
 * 
 * Smart context management with token counting, compression, and priority tagging.
 * Inspired by OpenClaw context manager and mimiclaw memory system.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/**
 * @brief Message role types
 */
typedef enum {
    CONTEXT_ROLE_SYSTEM = 0,
    CONTEXT_ROLE_USER,
    CONTEXT_ROLE_ASSISTANT,
    CONTEXT_ROLE_TOOL
} context_role_t;

/**
 * @brief Message priority levels (higher = more important, kept longer)
 */
typedef enum {
    CONTEXT_PRIORITY_LOW = 1,
    CONTEXT_PRIORITY_NORMAL = 5,
    CONTEXT_PRIORITY_HIGH = 8,
    CONTEXT_PRIORITY_CRITICAL = 10  // System messages, never auto-deleted
} context_priority_t;

/**
 * @brief Single message in context
 */
typedef struct {
    context_role_t role;
    context_priority_t priority;
    char* content;
    size_t token_count;
    TickType_t timestamp;
    bool is_compressed;
} context_message_t;

/**
 * @brief Context manager configuration
 */
typedef struct {
    size_t max_tokens;          // Maximum tokens in context window
    size_t max_messages;        // Maximum number of messages
    float compression_threshold;// Trigger compression at this % (0.0-1.0)
    bool enable_auto_compress;  // Auto-compress when threshold reached
} context_config_t;

/**
 * @brief Context manager instance
 */
typedef struct {
    context_config_t config;
    context_message_t* messages;
    size_t message_count;
    size_t capacity;
    size_t total_tokens;
    SemaphoreHandle_t mutex;
    bool initialized;
} context_manager_t;

/**
 * @brief Token estimation callback (implement tiktoken or similar)
 * @param content Text to count tokens for
 * @return Estimated token count
 */
typedef size_t (*token_counter_func_t)(const char* content);

/**
 * @brief Initialize context manager
 * @param config Configuration parameters
 * @return Pointer to context manager or NULL on failure
 */
context_manager_t* ctx_mgr_init(const context_config_t* config);

/**
 * @brief Deinitialize context manager and free resources
 * @param mgr Context manager instance
 */
void ctx_mgr_deinit(context_manager_t* mgr);

/**
 * @brief Add a message to context
 * @param mgr Context manager
 * @param role Message role
 * @param content Message content
 * @param priority Message priority
 * @return ESP_OK on success
 */
esp_err_t ctx_mgr_add_message(context_manager_t* mgr, 
                              context_role_t role,
                              const char* content,
                              context_priority_t priority);

/**
 * @brief Get current token count
 * @param mgr Context manager
 * @return Total tokens in context
 */
size_t ctx_mgr_get_token_count(context_manager_t* mgr);

/**
 * @brief Get message count
 * @param mgr Context manager
 * @return Number of messages
 */
size_t ctx_mgr_get_message_count(context_manager_t* mgr);

/**
 * @brief Compress context by removing low-priority messages
 * @param mgr Context manager
 * @param target_tokens Target token count after compression
 * @return ESP_OK on success
 */
esp_err_t ctx_mgr_compress(context_manager_t* mgr, size_t target_tokens);

/**
 * @brief Set custom token counter function
 * @param mgr Context manager
 * @param counter_func Token counting function
 */
void ctx_mgr_set_token_counter(context_manager_t* mgr, token_counter_func_t counter_func);

/**
 * @brief Get formatted prompt for LLM API
 * @param mgr Context manager
 * @param buffer Output buffer
 * @param buffer_len Buffer length
 * @return ESP_OK on success
 */
esp_err_t ctx_mgr_get_formatted_prompt(context_manager_t* mgr, char* buffer, size_t buffer_len);

/**
 * @brief Clear all messages from context
 * @param mgr Context manager
 * @return ESP_OK on success
 */
esp_err_t ctx_mgr_clear(context_manager_t* mgr);

/**
 * @brief Remove oldest low-priority message
 * @param mgr Context manager
 * @return ESP_OK on success
 */
esp_err_t ctx_mgr_remove_oldest(context_manager_t* mgr);

/**
 * @brief Get memory usage statistics
 * @param mgr Context manager
 * @param heap_size Output: heap memory used
 * @param psram_size Output: PSRAM memory used
 */
void ctx_mgr_get_memory_stats(context_manager_t* mgr, size_t* heap_size, size_t* psram_size);

#ifdef __cplusplus
}
#endif
