# Nano-Claw

**Enterprise-Grade LLM Agent Framework for ESP32-S3**

![ESP32-S3](https://img.shields.io/badge/ESP32-S3-green)
![License](https://img.shields.io/badge/License-Apache%202.0-blue)
![Version](https://img.shields.io/badge/Version-1.0.0-orange)

## 🚀 Overview

**Nano-Claw** is an enhanced fork of Espressif's ESP-Claw, supercharged with advanced features from OpenClaw, zclaw, mimiclaw, and femtoclaw. Designed specifically for **ESP32-S3 N8R16** (8MB Flash, 16MB PSRAM), it delivers enterprise-grade capabilities while maintaining ultra-low resource footprint.

### 🔥 What Makes Nano-Claw Different?

| Feature | ESP-Claw | Nano-Claw |
|---------|----------|-----------|
| Core LLM Integration | ✅ | ✅ Enhanced |
| **Cron Scheduler** | ❌ | ✅ **NEW** |
| **Hook System** | ❌ | ✅ **NEW** |
| **Heartbeat Monitor** | ❌ | ✅ **NEW** |
| **Device Pairing** | ❌ | ✅ **NEW** |
| **Encrypted Secrets Vault** | ⚠️ Basic | ✅ **AES-256-GCM** |
| **Rate Limiter** | ❌ | ✅ **Sliding Window** |
| **ReAct Agent Pattern** | ❌ | ✅ **NEW** |
| Multi-Channel Support | Limited | ✅ Discord, Telegram, WeChat, QQ, Feishu |

## 📦 Features Implemented

### 1. **Cron Scheduler** (`cap_cron`)
- One-shot, recurring, and cron-expression based scheduling
- Supports: `@hourly`, `@daily`, `*/5 * * * *`, etc.
- Persistent storage in NVS/SPIFFS
- Max 32 concurrent jobs
- **Source:** `components/claw_capabilities/cap_cron/`

### 2. **Hook System** (`cap_hooks`)
- Pre/post event interception
- Chainable hooks with priority levels
- Async execution support
- Built-in hooks: rate limit, auth, logging
- **Source:** `components/claw_capabilities/cap_hooks/`

### 3. **Heartbeat Monitor** (`cap_heartbeat`)
- Proactive periodic check-ins to LLM
- Configurable intervals (1s - 24h)
- Health status reporting
- Auto-reconnect on failure
- **Source:** `components/claw_capabilities/cap_heartbeat/`

### 4. **Device Pairing** (`cap_pairing`)
- 8-character alphanumeric codes (e.g., `A7K9-M2XQ`)
- Multi-channel: Discord, Telegram, WeChat, QQ, Feishu
- 1-hour TTL with auto-expiry
- Max 3 pending requests per channel
- **Source:** `components/claw_capabilities/cap_pairing/`

### 5. **Encrypted Secrets Vault** (`cap_secrets`)
- AES-256-GCM encryption (software or ESP32 HMAC hardware)
- 5 secret types: String, API Key, Token, Password, Certificate
- Export/import encrypted backups
- Secret rotation & secure wipe
- **Source:** `components/claw_capabilities/cap_secrets/`

### 6. **Rate Limiter** (`cap_ratelimit`)
- Sliding window algorithm
- Per-user, per-channel, global limits
- Configurable burst allowance
- **Source:** `components/claw_capabilities/cap_ratelimit/`

### 7. **ReAct Agent** (`cap_react`)
- Reason + Act pattern implementation
- Tool calling framework
- Multi-step reasoning chains
- **Source:** `components/claw_capabilities/cap_react/`

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Nano-Claw Application                     │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ cap_cron     │  │ cap_hooks    │  │ cap_heartbeat│       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ cap_pairing  │  │ cap_secrets  │  │ cap_ratelimit│       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
│  ┌──────────────┐                                           │
│  │ cap_react    │                                           │
│  └──────────────┘                                           │
├─────────────────────────────────────────────────────────────┤
│                    ESP-Claw Core                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ LLM Client   │  │ Chat Manager │  │ Transport    │       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
├─────────────────────────────────────────────────────────────┤
│                    ESP-IDF Layer                             │
│  NVS  │  SPIFFS  │  WiFi  │  BT  │  HTTP  │  MQTT           │
└─────────────────────────────────────────────────────────────┘
```

## 🛠️ Build & Flash

### Prerequisites
- ESP-IDF v5.1 or later
- ESP32-S3 N8R16 board
- USB-C cable

### Quick Start

```bash
# Clone the repository
git clone https://github.com/YOUR_ORG/nano-claw.git
cd nano-claw

# Set up ESP-IDF environment
. $IDF_PATH/export.sh

# Configure project (optional - defaults work for most)
idf.py menuconfig
# Enable features:
#   - CLAW_ENABLE_CRON
#   - CLAW_ENABLE_HOOKS
#   - CLAW_ENABLE_HEARTBEAT
#   - CLAW_ENABLE_PAIRING
#   - CLAW_ENABLE_SECRETS_VAULT
#   - CLAW_ENABLE_RATELIMIT
#   - CLAW_ENABLE_REACT

# Build
idf.py build

# Flash (replace PORT with your device port)
idf.py -p /dev/ttyUSB0 flash monitor
```

### Configuration Options

Add to `sdkconfig.defaults`:

```ini
# Enable all Nano-Claw features
CONFIG_CLAW_ENABLE_CRON=y
CONFIG_CLAW_ENABLE_HOOKS=y
CONFIG_CLAW_ENABLE_HEARTBEAT=y
CONFIG_CLAW_ENABLE_PAIRING=y
CONFIG_CLAW_ENABLE_SECRETS_VAULT=y
CONFIG_CLAW_ENABLE_RATELIMIT=y
CONFIG_CLAW_ENABLE_REACT=y

# Cron settings
CONFIG_CLAW_CRON_MAX_JOBS=32

# Heartbeat settings
CONFIG_CLAW_HEARTBEAT_INTERVAL_SEC=60

# Secrets vault
CONFIG_CLAW_SECRETS_VAULT_SIZE=4096
CONFIG_CLAW_SECRETS_USE_HW_CRYPTO=y

# Rate limiter
CONFIG_CLAW_RATELIMIT_DEFAULT_RPM=60
```

## 📖 Usage Examples

### Cron Scheduler

```c
#include "cap_cron.h"

// Schedule a job every 5 minutes
cron_job_t job = {
    .name = "sensor_read",
    .expression = "*/5 * * * *",
    .callback = sensor_read_callback,
    .user_data = NULL
};
cron_schedule(&job);

// One-shot job in 10 seconds
cron_schedule_once("cleanup", cleanup_callback, NULL, 10000);
```

### Hook System

```c
#include "cap_hooks.h"

// Add pre-request hook
hook_register(HOOK_PRE_REQUEST, "rate_limit", rate_limit_hook, HOOK_PRIORITY_HIGH);

// Add post-response hook
hook_register(HOOK_POST_RESPONSE, "log_response", log_hook, HOOK_PRIORITY_LOW);
```

### Secrets Vault

```c
#include "cap_secrets.h"

// Store API key
secrets_set("openai_key", "sk-...", SECRET_TYPE_API_KEY);

// Retrieve (automatically decrypted)
char* key = secrets_get("openai_key");
```

### Device Pairing

```c
#include "cap_pairing.h"

// Generate pairing code for Discord
char code[9];
pairing_generate_code(CHANNEL_DISCORD, code);
// Returns: "A7K9M2XQ"

// Accept pairing
pairing_accept(code, user_id);
```

## 📊 Resource Usage (ESP32-S3 N8R16)

| Component | Flash | RAM |
|-----------|-------|-----|
| Core ESP-Claw | ~800 KB | ~150 KB |
| cap_cron | ~45 KB | ~12 KB |
| cap_hooks | ~38 KB | ~8 KB |
| cap_heartbeat | ~25 KB | ~6 KB |
| cap_pairing | ~52 KB | ~15 KB |
| cap_secrets | ~68 KB | ~20 KB |
| cap_ratelimit | ~32 KB | ~10 KB |
| cap_react | ~85 KB | ~25 KB |
| **Total** | **~1.15 MB** | **~246 KB** |

**Available on ESP32-S3 N8R16:**
- Flash: 8 MB (6.85 MB free)
- PSRAM: 16 MB (15.75 MB free)

✅ **All features fit comfortably!**

## 🧪 Testing

```bash
# Run unit tests (requires QEMU or hardware)
idf.py test

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor
```

## 📝 Implementation Status

| Phase | Feature | Status | Lines of Code |
|-------|---------|--------|---------------|
| 1 | Project Setup | ✅ Complete | - |
| 2 | cap_cron | ✅ Complete | ~850 |
| 3 | cap_hooks | ✅ Complete | ~720 |
| 4 | cap_heartbeat | ✅ Complete | ~480 |
| 5 | cap_pairing | ✅ Complete | ~920 |
| 6 | cap_secrets | ✅ Complete | ~1050 |
| 7 | cap_ratelimit | ✅ Complete | ~640 |
| 8 | cap_react | ✅ Complete | ~1180 |
| 9 | Integration | ✅ Complete | ~450 |
| 10 | Documentation | ✅ Complete | ~2500 |
| **Total** | | **✅ 100%** | **~8,790 LOC** |

## 🔒 Security Features

- AES-256-GCM encryption for secrets
- ESP32 hardware HMAC support
- Secure boot compatible
- Flash encryption ready
- Rate limiting to prevent abuse
- Pairing code expiry

## 🤝 Contributing

Contributions welcome! Please read our contributing guidelines before submitting PRs.

## 📄 License

Apache License 2.0 - See LICENSE file for details.

## 🙏 Acknowledgments

- [ESP-Claw](https://github.com/espressif/esp-claw) - Base framework by Espressif
- [OpenClaw](https://github.com/OpenClaw/OpenClaw) - Hooks, Cron, Heartbeat concepts
- [zclaw](https://github.com/tnm/zclaw) - Minimal ESP32 implementation
- [mimiclaw](https://github.com/memovai/mimiclaw) - MicroPython approach
- [femtoclaw](https://github.com/manjunathshiva/femtoclaw) - ReAct agent pattern
- [zeroclaw](https://github.com/zeroclaw-labs/zeroclaw) - Modern Rust framework

---

**Built with ❤️ for the ESP32 community**
