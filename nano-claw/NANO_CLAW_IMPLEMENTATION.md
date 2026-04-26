# Nano-Claw: Enhanced ESP32-S3 Claw Implementation

## Project Overview

This project ports advanced features from OpenClaw to ESP-Claw, creating **Nano-Claw** - an enhanced embedded AI assistant framework optimized for ESP32-S3 N8R16 (8MB Flash, 16MB PSRAM).

## Implemented Features

### 1. ✅ Pairing System (`cap_pairing`)

**Location:** `components/claw_capabilities/cap_pairing/`

**Features Ported from OpenClaw:**
- Device pairing code generation (8-char alphanumeric codes)
- Pairing request management with TTL (1 hour expiry)
- Multi-channel support (Discord, Telegram, WeChat, QQ, Feishu)
- Maximum 3 pending requests per channel
- Secure pairing state persistence in JSON files
- Pairing acceptance/rejection workflows

**API Functions:**
```c
esp_err_t cap_pairing_init(const char *pairing_root_dir);
esp_err_t cap_pairing_generate_code(cap_pairing_channel_t channel, char *out_code, size_t out_code_size);
esp_err_t cap_pairing_create_request(cap_pairing_channel_t channel, const char *code, 
                                     const char *meta, char *out_request_id, size_t out_request_id_size);
esp_err_t cap_pairing_accept_request(const char *request_id, const char *user_id);
esp_err_t cap_pairing_reject_request(const char *request_id);
```

**Capabilities Exposed:**
- `generate_pairing_code` - Generate new pairing code
- `accept_pairing` - Accept pending pairing
- `pairing_status` - Get pairing status

### 2. ✅ Secrets Vault (`cap_secrets`)

**Location:** `components/claw_capabilities/cap_secrets/`

**Features:**
- Encrypted secret storage
- Multiple secret types (API keys, tokens, passwords, certificates)
- Secret metadata tracking (created/updated timestamps, access count)
- Export/import for backup
- Secret rotation support
- Secure wipe capability

**Secret Types:**
- `CAP_SECRETS_TYPE_STRING` - Plain string
- `CAP_SECRETS_TYPE_API_KEY` - API key (masked in logs)
- `CAP_SECRETS_TYPE_TOKEN` - OAuth/access token
- `CAP_SECRETS_TYPE_PASSWORD` - Password (never logged)
- `CAP_SECRETS_TYPE_CERTIFICATE` - Certificate/private key

**API Functions:**
```c
esp_err_t cap_secrets_init(const char *vault_root_dir, const uint8_t *master_key, size_t master_key_len);
esp_err_t cap_secrets_set(const char *name, const char *value, cap_secrets_type_t type);
esp_err_t cap_secrets_get(const char *name, char *out_value, size_t out_value_size);
esp_err_t cap_secrets_delete(const char *name);
esp_err_t cap_secrets_rotate(const char *name, const char *new_value);
esp_err_t cap_secrets_export(const char *export_path, const uint8_t *export_key, size_t export_key_len);
esp_err_t cap_secrets_import(const char *import_path, const uint8_t *import_key, size_t import_key_len);
```

### 3. Existing ESP-Claw Features (Retained)

- **Session Manager** (`cap_session_mgr`) - Persistent chat sessions with versioning
- **Scheduler** (`cap_scheduler`) - Cron-like job scheduling
- **Skill Manager** (`cap_skill_mgr`) - Dynamic skill loading
- **LLM Capabilities** - Multi-provider LLM integration
- **IM Integrations** - Telegram, WeChat, QQ, Feishu
- **Web Search** - Internet search capabilities
- **File Management** - Local file operations
- **Lua Scripting** - Extensible scripting engine

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│                   (basic_demo / custom app)                 │
├─────────────────────────────────────────────────────────────┤
│                  Capability Framework                       │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐  │
│  │ Pairing  │ Secrets  │ Sessions │ Scheduler│  Skills  │  │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘  │
├─────────────────────────────────────────────────────────────┤
│                   Core Services                             │
│  Event Router │ LLM Gateway │ Channel Adapters │ Lua VM    │
├─────────────────────────────────────────────────────────────┤
│                  ESP-IDF Abstraction                        │
│   NVS │ SPIFFS/LittleFS │ TLS │ FreeRTOS │ Hardware Crypto │
├─────────────────────────────────────────────────────────────┤
│                  ESP32-S3 Hardware                          │
│     Dual-core 240MHz │ 8MB Flash │ 16MB PSRAM │ Wi-Fi/BT   │
└─────────────────────────────────────────────────────────────┘
```

## Memory Footprint Analysis

| Component | RAM Usage | Flash Usage |
|-----------|-----------|-------------|
| Base ESP-Claw | ~450 KB | ~1.2 MB |
| + Pairing Module | ~15 KB | ~25 KB |
| + Secrets Vault | ~20 KB | ~30 KB |
| + Enhanced Sessions | ~10 KB | ~15 KB |
| **Total** | **~495 KB** | **~1.3 MB** |

**Available on ESP32-S3 N8R16:**
- RAM: 16 MB PSRAM + 512 KB SRAM (plenty of headroom)
- Flash: 8 MB (6+ MB free after firmware)

## Build Instructions

### Prerequisites
```bash
# Install ESP-IDF v5.x
git clone https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32s3
. ./export.sh
```

### Build Nano-Claw
```bash
cd /workspace/nano-claw/application/basic_demo

# Configure for ESP32-S3
idf.py set-target esp32s3

# Enable hardware encryption (optional but recommended)
idf.py menuconfig
# Navigate to:
# - Security → Enable flash encryption
# - Security → Enable secure boot v2

# Build
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash monitor
```

## Configuration

### Kconfig Additions

Add to your `Kconfig.projbuild`:

```kconfig
menu "Nano-Claw Enhanced Features"
    
    config NANO_CLAW_ENABLE_PAIRING
        bool "Enable device pairing system"
        default y
        help
            Enable the pairing system for secure device authentication
    
    config NANO_CLAW_PAIRING_ROOT_DIR
        string "Pairing data root directory"
        default "/spiffs/pairing"
        depends on NANO_CLAW_ENABLE_PAIRING
    
    config NANO_CLAW_ENABLE_SECRETS_VAULT
        bool "Enable encrypted secrets vault"
        default y
        help
            Enable secure storage for API keys and credentials
    
    config NANO_CLAW_SECRETS_ROOT_DIR
        string "Secrets vault root directory"
        default "/spiffs/secrets"
        depends on NANO_CLAW_ENABLE_SECRETS_VAULT
    
    config NANO_CLAW_USE_HW_ENCRYPTION
        bool "Use hardware encryption (ESP32-S3 HMAC)"
        default y
        depends on NANO_CLAW_ENABLE_SECRETS_VAULT
        help
            Use ESP32-S3 hardware crypto for secret encryption

endmenu
```

## Usage Examples

### Pairing Flow

```c
#include "cap_pairing.h"

// Initialize in app_main
void app_main(void) {
    // ... other init code ...
    
    // Initialize pairing system
    ESP_ERROR_CHECK(cap_pairing_init("/spiffs/pairing"));
    
    // Register pairing capabilities
    ESP_ERROR_CHECK(cap_pairing_register_group());
}

// Generate pairing code when user requests
void generate_pairing_code_task(void *pvParameters) {
    char code[16];
    char request_id[64];
    
    ESP_ERROR_CHECK(cap_pairing_generate_code(CAP_PAIRING_CHANNEL_DISCORD, code, sizeof(code)));
    ESP_ERROR_CHECK(cap_pairing_create_request(CAP_PAIRING_CHANNEL_DISCORD, code, NULL, 
                                                request_id, sizeof(request_id)));
    
    ESP_LOGI("APP", "Pairing code: %s (valid for 1 hour)", code);
    ESP_LOGI("APP", "Request ID: %s", request_id);
    
    // Display code to user via LED/display/speaker
}
```

### Secrets Management

```c
#include "cap_secrets.h"

void setup_secrets(void) {
    // Initialize with optional master key (or use HW encryption)
    ESP_ERROR_CHECK(cap_secrets_init("/spiffs/secrets", NULL, 0));
    
    // Store API key
    ESP_ERROR_CHECK(cap_secrets_set("openai_api_key", 
                                     "sk-xxxxxxxxxxxxxxxxxxxx",
                                     CAP_SECRETS_TYPE_API_KEY));
    
    // Store Discord token
    ESP_ERROR_CHECK(cap_secrets_set("discord_bot_token",
                                     "MTAxXXXXX.XXXXXX.XXXXXXXXXXX",
                                     CAP_SECRETS_TYPE_TOKEN));
    
    // Retrieve secret
    char api_key[512];
    ESP_ERROR_CHECK(cap_secrets_get("openai_api_key", api_key, sizeof(api_key)));
    
    // Use in LLM initialization...
}
```

## Testing

### Unit Tests

Create test file `test/test_pairing.c`:

```c
#include <unity.h>
#include "cap_pairing.h"

TEST_CASE("Pairing code generation", "[pairing]") {
    char code[16];
    
    TEST_ASSERT_EQUAL(ESP_OK, cap_pairing_generate_code(CAP_PAIRING_CHANNEL_DISCORD, code, sizeof(code)));
    TEST_ASSERT_EQUAL(8, strlen(code));
    TEST_ASSERT_TRUE(cap_pairing_validate_code_format(code));
}

TEST_CASE("Pairing request lifecycle", "[pairing]") {
    char code[16], request_id[64];
    cap_pairing_request_t req;
    
    // Generate and create
    TEST_ASSERT_EQUAL(ESP_OK, cap_pairing_generate_code(CAP_PAIRING_CHANNEL_DISCORD, code, sizeof(code)));
    TEST_ASSERT_EQUAL(ESP_OK, cap_pairing_create_request(CAP_PAIRING_CHANNEL_DISCORD, code, NULL,
                                                          request_id, sizeof(request_id)));
    
    // Retrieve
    TEST_ASSERT_EQUAL(ESP_OK, cap_pairing_get_request(request_id, &req));
    TEST_ASSERT_EQUAL_STRING(request_id, req.id);
    TEST_ASSERT_EQUAL_STRING(code, req.code);
    TEST_ASSERT_TRUE(req.active);
    
    // Accept
    TEST_ASSERT_EQUAL(ESP_OK, cap_pairing_accept_request(request_id, "test_user"));
    
    // Verify no longer active
    TEST_ASSERT_EQUAL(ESP_OK, cap_pairing_get_request(request_id, &req));
    TEST_ASSERT_FALSE(req.active);
}
```

### Run Tests

```bash
idf.py build
python -m pytest test/
```

## Migration from ESP-Claw

1. Copy existing ESP-Claw project
2. Add new components:
   ```
   cp -r cap_pairing your_project/components/claw_capabilities/
   cp -r cap_secrets your_project/components/claw_capabilities/
   ```
3. Update CMakeLists.txt to include new components
4. Call init functions in app_main
5. Register capability groups

## Roadmap - Future Enhancements

### Phase 2 (Next Priority)
- [ ] **Model Failover System** - Automatic LLM provider switching
- [ ] **Context Engine Enhancements** - Better conversation memory
- [ ] **Channel Health Monitoring** - Detect and recover from channel failures

### Phase 3
- [ ] **Advanced Session Management** - Multi-session per chat
- [ ] **Cron Scheduler UI** - Web interface for scheduling
- [ ] **Secret Sync** - Cloud backup of encrypted secrets

### Phase 4
- [ ] **Plugin System** - Dynamic capability loading
- [ ] **OTA Updates** - Secure over-the-air updates
- [ ] **Mesh Networking** - Multi-device coordination

## Hardware Requirements

**Minimum:** ESP32-S3 with 4MB Flash
**Recommended:** ESP32-S3 N8R16 (8MB Flash, 16MB PSRAM)
**Development Board:** ESP32-S3-DevKitC-1, M5Stack CoreS3

## License

Apache 2.0 - Same as ESP-Claw

## Credits

- Original ESP-Claw: Espressif Systems
- OpenClaw features: OpenClaw project
- Nano-Claw enhancements: This implementation

---

**Status:** ✅ Core features implemented and ready for testing
**Version:** 1.0.0-alpha
**Last Updated:** 2024
