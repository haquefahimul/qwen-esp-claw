/**
 * @file cap_tokenizer.h
 * @brief Lightweight BPE-style Token Counter for Nano-Claw
 * 
 * Provides approximate token counting for LLM context management.
 * Uses simplified byte-pair encoding estimation optimized for ESP32.
 */

#ifndef CAP_TOKENIZER_H
#define CAP_TOKENIZER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Count tokens in a text string
 * @param text Input text
 * @return Estimated token count
 * 
 * Uses a heuristic: ~4 chars per token for English text.
 * More accurate for common LLM tokenizers (GPT, Claude, Llama).
 */
size_t tokenizer_count_tokens(const char* text);

/**
 * @brief Count tokens with Unicode awareness
 * @param text Input text (UTF-8)
 * @param len Length of text in bytes
 * @return Estimated token count
 */
size_t tokenizer_count_tokens_utf8(const char* text, size_t len);

/**
 * @brief Truncate text to fit within token limit
 * @param text Input text (modified in place)
 * @param max_tokens Maximum token count
 * @return Actual token count after truncation
 */
size_t tokenizer_truncate(char* text, size_t max_tokens);

/**
 * @brief Get character count approximation
 * @param text Input text
 * @return Character count
 */
size_t tokenizer_char_count(const char* text);

#ifdef __cplusplus
}
#endif

#endif // CAP_TOKENIZER_H
