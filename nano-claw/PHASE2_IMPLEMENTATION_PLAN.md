# 🚀 NANO-CLAW PHASE 2: ADVANCED FEATURES IMPLEMENTATION

## 🎯 Mission
Port **6 additional advanced features** from OpenClaw, zclaw, mimiclaw, and femtoclaw to create the most complete ESP32-S3 LLM agent framework.

---

## 📋 NEW FEATURES ADDED (Phase 2)

| # | Component | LOC | Source | Description |
|---|-----------|-----|--------|-------------|
| 8 | **cap_context** | ~450 | OpenClaw/mimiclaw | Smart context window management with token counting |
| 9 | **cap_tools** | ~520 | OpenClaw/femtoclaw | Dynamic tool registration & execution engine |
| 10 | **cap_stream** | ~380 | OpenClaw/zclaw | SSE streaming parser for real-time responses |
| 11 | **cap_fsm** | ~340 | femtoclaw/zclaw | Finite State Machine for conversation flows |
| 12 | **cap_rag_lite** | ~480 | mimiclaw/OpenClaw | Lightweight RAG with vector similarity (embedded) |
| 13 | **cap_power** | ~220 | zclaw | Power management & sleep/wake optimization |

**Total Phase 2 Code:** ~2,390 lines

---

## 🔥 FEATURE DETAILS

### 8. Context Engine (`cap_context`)
**Source:** OpenClaw context manager + mimiclaw memory system

**Features:**
- Sliding window context management
- Token counting (tiktoken-compatible estimation)
- Context compression triggers
- Message priority tagging (system > user > assistant)
- Automatic truncation strategies
- Support for 4K, 8K, 16K context windows

**API:**
```c
context_manager_t* ctx_mgr_create(size_t max_tokens);
esp_err_t ctx_mgr_add_message(context_manager_t* mgr, const char* role, const char* content);
esp_err_t ctx_mgr_compress(context_manager_t* mgr);
size_t ctx_mgr_get_token_count(context_manager_t* mgr);
const char* ctx_mgr_get_formatted_prompt(context_manager_t* mgr);
```

**Memory Usage:** ~12KB RAM (configurable buffer)

---

### 9. Tool Engine (`cap_tools`)
**Source:** OpenClaw tools + femtoclaw ReAct extensions

**Features:**
- Dynamic tool registration (max 32 tools)
- Parameter schema validation
- Async tool execution
- Built-in tools: `get_time`, `get_weather`, `calculate`, `search_memory`
- Custom tool callback registration
- Tool result caching (TTL-based)

**API:**
```c
esp_err_t tool_register(const char* name, const char* description, tool_func_t callback);
esp_err_t tool_execute(const char* name, const char* json_args, char* output, size_t output_len);
esp_err_t tool_unregister(const char* name);
tool_info_t* tool_list(size_t* count);
```

**Example Tool:**
```c
static esp_err_t get_time_tool(const char* args, char* output, size_t len) {
    time_t now = time(NULL);
    strftime(output, len, "%Y-%m-%d %H:%M:%S", localtime(&now));
    return ESP_OK;
}
// Register: tool_register("get_time", "Get current time", get_time_tool);
```

---

### 10. Stream Parser (`cap_stream`)
**Source:** OpenClaw SSE handler + zclaw minimal parser

**Features:**
- Server-Sent Events (SSE) parsing
- Chunked transfer decoding
- Delta accumulation for complete messages
- Event type detection (`message`, `error`, `done`)
- Buffer overflow protection
- Callback-based streaming

**API:**
```c
stream_parser_t* stream_parser_create(stream_callback_t cb, void* user_data);
esp_err_t stream_parser_feed(stream_parser_t* parser, const char* data, size_t len);
esp_err_t stream_parser_finalize(stream_parser_t* parser);
void stream_parser_destroy(stream_parser_t* parser);
```

**Event Types:**
- `STREAM_EVENT_DELTA` - Partial content
- `STREAM_EVENT_COMPLETE` - Full message
- `STREAM_EVENT_ERROR` - Error occurred
- `STREAM_EVENT_DONE` - Stream finished

---

### 11. Finite State Machine (`cap_fsm`)
**Source:** femtoclaw state machine + zclaw flow control

**Features:**
- State definition & transitions
- Guard conditions for transitions
- Entry/exit actions per state
- Nested state support (2 levels)
- Timeout-based auto-transitions
- Visual state diagram generation (text-based)

**States:**
- `STATE_IDLE` - Waiting for input
- `STATE_LISTENING` - Processing input
- `STATE_THINKING` - LLM reasoning
- `STATE_ACTING` - Executing tools
- `STATE_RESPONDING` - Sending response
- `STATE_ERROR` - Error handling

**API:**
```c
fsm_t* fsm_create(const char* name);
esp_err_t fsm_add_state(fsm_t* fsm, const char* state_name, state_entry_t entry, state_exit_t exit);
esp_err_t fsm_add_transition(fsm_t* fsm, const char* from, const char* to, guard_func_t guard);
esp_err_t fsm_trigger(fsm_t* fsm, const char* event);
const char* fsm_get_current_state(fsm_t* fsm);
```

---

### 12. RAG Lite (`cap_rag_lite`)
**Source:** mimiclaw embedded memory + OpenClaw knowledge base

**Features:**
- Embedded vector storage (max 512 vectors, 384 dimensions)
- TF-IDF + cosine similarity (no external dependencies)
- Document chunking & indexing
- Incremental updates
- Persistence to SPIFFS/LittleFS
- Query top-K retrieval

**API:**
```c
rag_engine_t* rag_create(size_t max_documents);
esp_err_t rag_add_document(rag_engine_t* rag, const char* doc_id, const char* content);
esp_err_t rag_index(rag_engine_t* rag);
rag_result_t* rag_query(rag_engine_t* rag, const char* query, size_t top_k);
esp_err_t rag_save(rag_engine_t* rag, const char* path);
esp_err_t rag_load(rag_engine_t* rag, const char* path);
```

**Performance:**
- Indexing: ~50ms per document (ESP32-S3 @ 240MHz)
- Query: ~20ms for 512 documents
- Memory: ~256KB for 512 docs

---

### 13. Power Manager (`cap_power`)
**Source:** zclaw power optimization + ESP32 deep sleep patterns

**Features:**
- Dynamic frequency scaling
- Light sleep between requests
- Deep sleep with RTC wake
- WiFi power save modes
- Battery level monitoring (ADC)
- Wake sources: GPIO, timer, ULP

**Power Modes:**
- `POWER_MODE_ACTIVE` - 240MHz, WiFi on (~250mA)
- `POWER_MODE_LIGHT_SLEEP` - CPU paused, RTC on (~50mA)
- `POWER_MODE_DEEP_SLEEP` - Most peripherals off (~10µA)

**API:**
```c
esp_err_t power_init(power_config_t* config);
esp_err_t power_set_mode(power_mode_t mode);
esp_err_t power_enter_light_sleep(uint32_t sleep_ms);
esp_err_t power_enter_deep_sleep(uint32_t sleep_ms, gpio_num_t wake_pin);
int power_get_battery_level(void);
esp_err_t power_enable_wifi_ps_mode(void);
```

**Battery Monitoring:**
- ADC1 Channel 0 (GPIO1)
- Voltage divider calibration
- Percentage estimation with discharge curve

---

## 📊 COMBINED RESOURCE USAGE (Phase 1 + Phase 2)

| Component | Flash | RAM |
|-----------|-------|-----|
| Phase 1 (7 components) | ~288 KB | ~45 KB |
| Phase 2 (6 components) | ~312 KB | ~68 KB |
| **TOTAL** | **~600 KB** | **~113 KB** |

**ESP32-S3 N8R16 Capacity:**
- Flash: 8MB - 600KB = **7.4 MB FREE** (92.5%)
- PSRAM: 16MB - 113KB = **15.9 MB FREE** (99.3%)

✅ **ALL 13 FEATURES FIT COMFORTABLY!**

---

## 🏗️ ARCHITECTURE UPDATE

```
┌─────────────────────────────────────────────────────────────┐
│                    NANO-CLAW AGENT                          │
├─────────────────────────────────────────────────────────────┤
│  Application Layer (User Code)                              │
├─────────────────────────────────────────────────────────────┤
│  Orchestrator (app_claw.c)                                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                  │
│  │   FSM    │──│  Context │──│  Stream  │                  │
│  └──────────┘  └──────────┘  └──────────┘                  │
├─────────────────────────────────────────────────────────────┤
│  Capability Layer (13 Components)                           │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐   │
│  │ Cron   │ │ Hooks  │ │ Heart  │ │ Pair   │ │ Secret │   │
│  └────────┘ └────────┘ └────────┘ └────────┘ └────────┘   │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐   │
│  │ Rate   │ │ React  │ │ Context│ │ Tools  │ │ Stream │   │
│  └────────┘ └────────┘ └────────┘ └────────┘ └────────┘   │
│  ┌────────┐ ┌────────┐ ┌────────┐                          │
│  │ FSM    │ │ RAG    │ │ Power  │                          │
│  └────────┘ └────────┘ └────────┘                          │
├─────────────────────────────────────────────────────────────┤
│  Core Services (ESP-IDF)                                    │
│  WiFi │ HTTP │ TLS │ JSON │ NVS │ SPIFFS │ HMAC │ ADC      │
└─────────────────────────────────────────────────────────────┘
```

---

## 📝 IMPLEMENTATION CHECKLIST

### Phase 1 ✅ (COMPLETE)
- [x] cap_cron
- [x] cap_hooks
- [x] cap_heartbeat
- [x] cap_pairing
- [x] cap_secrets
- [x] cap_ratelimit
- [x] cap_react

### Phase 2 🚧 (IN PROGRESS)
- [ ] cap_context
- [ ] cap_tools
- [ ] cap_stream
- [ ] cap_fsm
- [ ] cap_rag_lite
- [ ] cap_power

---

## 🧪 TESTING STRATEGY

### Unit Tests (Each Component)
```c
// Example: cap_context test
TEST_CASE("Context manager handles token limit", "[context]") {
    context_manager_t* mgr = ctx_mgr_create(1000);
    TEST_ASSERT_NOT_NULL(mgr);
    
    // Add messages exceeding limit
    ctx_mgr_add_message(mgr, "user", long_content);
    TEST_ASSERT_UINT_LESS_OR_EQUAL(1000, ctx_mgr_get_token_count(mgr));
    
    ctx_mgr_destroy(mgr);
}
```

### Integration Tests
- Cron + Hooks interaction
- Tools + Context integration
- FSM + Stream coordination
- RAG + Context retrieval
- Power + Heartbeat optimization

### Hardware Tests (ESP32-S3 N8R16)
- Deep sleep current measurement
- WiFi reconnection after sleep
- RAG indexing performance
- Stream parsing under load
- Multi-day stability test

---

## 🚀 NEXT STEPS

1. **Implement Phase 2 components** (this document guides the code)
2. **Create CMakeLists.txt** for each new component
3. **Update main CMakeLists.txt** to include all 13 components
4. **Write unit tests** using Unity framework
5. **Build & flash** to ESP32-S3 N8R16
6. **Run hardware tests** and measure power consumption
7. **Document API** with Doxygen comments
8. **Create example projects** for each feature

---

## 📚 REFERENCES

- **OpenClaw:** https://github.com/OpenClaw/OpenClaw
- **zclaw:** https://github.com/tnm/zclaw
- **mimiclaw:** https://github.com/memovai/mimiclaw
- **femtoclaw:** https://github.com/manjunathshiva/femtoclaw
- **zeroclaw:** https://github.com/zeroclaw-labs/zeroclaw
- **ESP-Claw:** https://github.com/espressif/esp-claw

---

**Status:** Phase 2 Ready for Implementation  
**Target:** ESP32-S3 N8R16 (8MB Flash, 16MB PSRAM)  
**Version:** Nano-Claw v2.0.0 (Planned)
