/**
 * @file cap_tokenizer.c
 * @brief Implementation of Lightweight Token Counter
 */

#include "cap_tokenizer.h"
#include <string.h>
#include <ctype.h>

// Heuristic constants
#define AVG_CHARS_PER_TOKEN 4
#define WORD_BOUNDARY_PENALTY 1

static inline bool is_word_boundary(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r' || 
           c == '.' || c == ',' || c == '!' || c == '?' ||
           c == ';' || c == ':' || c == '(' || c == ')' ||
           c == '[' || c == ']' || c == '{' || c == '}';
}

size_t tokenizer_count_tokens(const char* text) {
    if (!text || !*text) return 0;
    
    size_t len = strlen(text);
    return tokenizer_count_tokens_utf8(text, len);
}

size_t tokenizer_count_tokens_utf8(const char* text, size_t len) {
    if (!text || len == 0) return 0;
    
    size_t token_count = 0;
    size_t word_len = 0;
    bool in_word = false;
    
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)text[i];
        
        // UTF-8 continuation byte
        if ((c & 0xC0) == 0x80) {
            if (in_word) word_len++;
            continue;
        }
        
        // ASCII or UTF-8 start byte
        if (is_word_boundary((char)c)) {
            if (in_word && word_len > 0) {
                // End of word - count tokens
                token_count += (word_len + AVG_CHARS_PER_TOKEN - 1) / AVG_CHARS_PER_TOKEN;
                if (word_len > 3) token_count += WORD_BOUNDARY_PENALTY;
            }
            in_word = false;
            word_len = 0;
        } else {
            in_word = true;
            word_len++;
        }
    }
    
    // Handle last word
    if (in_word && word_len > 0) {
        token_count += (word_len + AVG_CHARS_PER_TOKEN - 1) / AVG_CHARS_PER_TOKEN;
    }
    
    // Minimum 1 token for non-empty text
    return token_count > 0 ? token_count : 1;
}

size_t tokenizer_truncate(char* text, size_t max_tokens) {
    if (!text || max_tokens == 0) {
        if (text) text[0] = '\0';
        return 0;
    }
    
    size_t len = strlen(text);
    size_t current_tokens = tokenizer_count_tokens_utf8(text, len);
    
    if (current_tokens <= max_tokens) {
        return current_tokens;
    }
    
    // Binary search for truncation point
    size_t low = 0, high = len;
    while (low < high) {
        size_t mid = (low + high + 1) / 2;
        size_t tokens = tokenizer_count_tokens_utf8(text, mid);
        
        if (tokens <= max_tokens) {
            low = mid;
        } else {
            high = mid - 1;
        }
    }
    
    // Truncate at word boundary if possible
    while (low > 0 && !is_word_boundary(text[low - 1])) {
        low--;
    }
    
    text[low] = '\0';
    return tokenizer_count_tokens_utf8(text, low);
}

size_t tokenizer_char_count(const char* text) {
    if (!text) return 0;
    
    size_t count = 0;
    while (*text) {
        unsigned char c = (unsigned char)*text;
        if ((c & 0xC0) != 0x80) {  // Count only UTF-8 start bytes
            count++;
        }
        text++;
    }
    return count;
}
