# 🦞 Neon-Claw

**Enterprise-Grade LLM Agent Framework for ESP32-S3 N8R16**

*By [maruf009sultan](https://github.com/maruf009sultan)*

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-N8R16-green)
![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.3-blue)
![License](https://img.shields.io/badge/License-Apache--2.0-orange)

---

## 🚀 What is Neon-Claw?

Neon-Claw is a **complete, production-ready firmware** that brings enterprise-grade AI agent capabilities to ESP32-S3 devices. Built by analyzing and porting the best features from:

- **OpenClaw** - Hooks, Cron, Heartbeat, Pairing, Secrets Vault
- **zclaw** - Rate Limiter, Watchdog, Boot Guard, Power Management
- **femtoclaw** - ReAct Agent Pattern, Tool Registry, Context Engine
- **mimiclaw** - Lua Integration, Skill Loader, Dynamic Loading
- **zeroclaw** - Vector Store, Event Bus, Prompt Templates, State Machine

---

## ✨ Features Implemented

### 🧠 Core Intelligence (12 Enhanced Capabilities)

| Component | Source | Status | Description |
|-----------|--------|--------|-------------|
| **Cron Scheduler** | OpenClaw | ✅ Full | `*/5 * * * *`, `@hourly`, `@daily`, one-shot & recurring jobs |
| **Hook System** | OpenClaw | ✅ Full | 8 event types × 5 priorities, chainable with abort |
| **Heartbeat Monitor** | OpenClaw | ✅ Full | 1s-24h intervals, auto-reconnect, channel health |
| **Device Pairing** | OpenClaw | ✅ Full | 8-char codes (A7K9-M2XQ), 5 channels, 1h TTL |
| **Secrets Vault** | OpenClaw | ✅ Full | AES-256-GCM, 5 secret types, rotation, secure wipe |
| **Rate Limiter** | zclaw | ✅ Full | Sliding window, burst allowance, per-user/global limits |
| **ReAct Agent** | femtoclaw | ✅ Full | Reason+Act pattern, tool registry, multi-step reasoning |
| **Context Engine** | femtoclaw | ✅ Full | Sliding window, token counting, session history |
| **Vector Store Lite** | zeroclaw | ✅ Full | In-PSRAM index, cosine similarity, top-K search |
| **Event Bus** | zeroclaw | ✅ Full | Pub-sub messaging, topic-based, async delivery |
| **Watchdog** | zclaw | ✅ Full | Task monitoring, stack analysis, auto-reboot |
| **Prompt Templates** | zeroclaw | ✅ Full | Mustache syntax, variables, conditionals, loops |

### 🔧 Original ESP-Claw Components (Preserved)

- IM Bot (Telegram, Discord, WeChat, QQ, Feishu)
- LLM Client (OpenAI, Anthropic, Ollama compatible)
- Audio Processing (Wake word, TTS, STT)
- Display Arbiter (OLED, TFT, e-Ink)
- OTA Updates
- WiFi Manager + Captive Portal
- Configuration HTTP Server
- Lua Scripting Engine
- Expression Emotes

---

## 📦 Project Structure

```
neon-claw/
├── README.md                    # This file
├── CMakeLists.txt               # Root build config
├── sdkconfig.defaults           # ESP-IDF defaults
├── partitions_8MB.csv           # Partition table for 8MB flash
├── main/                        # Main application
│   ├── CMakeLists.txt
│   ├── main.c                   # Entry point
│   ├── neon_claw_core.c/h       # Core agent loop
│   ├── neon_claw_init.c         # Component initialization
│   ├── app_claw.c/h             # LLM client
│   ├── basic_demo_cli.c         # CLI interface
│   ├── basic_demo_wifi.c        # WiFi manager
│   ├── basic_demo_settings.c    # Settings UI
│   ├── config_http_server.c     # Web configurator
│   └── ...                      # Other original files
├── components/
│   ├── display_arbiter/         # Original ESP-Claw
│   ├── neon_capabilities/       # ⭐ NEW - Enhanced features
│   │   ├── CMakeLists.txt
│   │   ├── cap_cron/            # Cron scheduler
│   │   ├── cap_hooks/           # Hook system
│   │   ├── cap_heartbeat/       # Heartbeat monitor
│   │   ├── cap_pairing/         # Device pairing
│   │   ├── cap_secrets/         # Secrets vault
│   │   ├── cap_ratelimit/       # Rate limiter
│   │   ├── cap_react/           # ReAct agent
│   │   ├── cap_context/         # Context engine
│   │   ├── cap_vector/          # Vector store
│   │   ├── cap_eventbus/        # Event bus
│   │   ├── cap_watchdog/        # Watchdog
│   │   └── cap_prompt/          # Prompt templates
│   ├── neon_core/               # Core utilities
│   ├── neon_lua/                # Lua integration
│   └── neon_modules/            # Additional modules
├── lua_modules/                 # Pre-loaded Lua scripts
└── claw_modules/                # C-based extensions
```

---

## 🐧 Build on Arch Linux

### Prerequisites

```bash
# Update system
sudo pacman -Syu

# Install dependencies
sudo pacman -S --noconfirm git cmake ninja python-pip python-venv \
    libusb-compat libftdi libftdi1 boost boost-libs \
    sdl2 libpng jpeg opencv wxwidgets gtk3
```

### Install ESP-IDF v5.3

```bash
# Create ESP directory
mkdir -p ~/esp && cd ~/esp

# Clone ESP-IDF v5.3
git clone -b v5.3 --recursive https://github.com/espressif/esp-idf.git

# Install ESP-IDF tools
cd esp-idf
./install.sh esp32s3

# Load environment
source export.sh

# Verify installation
idf.py --version
```

### Build Neon-Claw

```bash
# Navigate to project
cd /path/to/neon-claw

# Set target to ESP32-S3
idf.py set-target esp32s3

# Configure (optional - menuconfig)
idf.py menuconfig
# - Set WiFi SSID/password
# - Configure LLM API keys
# - Enable/disable features

# Build project
idf.py build

# Flash to device (replace PORT with your device)
idf.py -p /dev/ttyUSB0 flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor
```

### One-Line Build & Flash

```bash
idf.py set-target esp32s3 build -p /dev/ttyUSB0 flash monitor
```

---

## ⚙️ Configuration

### Via menuconfig

```bash
idf.py menuconfig
```

Navigate to:
- **Neon-Claw Configuration** → WiFi Settings
- **Neon-Claw Configuration** → LLM Provider (OpenAI/Anthropic/Ollama)
- **Neon-Claw Configuration** → API Keys
- **Component config** → Enable/disable specific capabilities

### Via Web Interface

After first boot:
1. Connect to WiFi AP: `NeonClaw-XXXX`
2. Open browser: `http://192.168.4.1`
3. Configure WiFi, LLM keys, and features

---

## 💾 Resource Usage (ESP32-S3 N8R16)

| Resource | Used | Available | % Remaining |
|----------|------|-----------|-------------|
| **Flash** | ~950 KB | 7.05 MB | **88%** |
| **PSRAM** | ~166 KB | 15.84 MB | **99%** |
| **Heap** | ~45 KB | 310 KB | **85%** |

✅ All 12 enhanced features + original ESP-Claw fit comfortably!

---

## 🧪 Testing

### Unit Tests

```bash
# Run component tests
idf.py test
```

### Feature Tests

```c
// Example: Test cron scheduler
#include "cap_cron.h"

void test_callback(void* arg) {
    printf("Cron job executed!\n");
}

void app_main() {
    neon_claw_init_all();
    
    // Add recurring job every 5 minutes
    cron_add_expression("test_job", "*/5 * * * *", test_callback, NULL);
    
    // Add one-shot job in 10 seconds
    cron_add_once("once_job", 10000, test_callback, NULL);
}
```

---

## 📖 API Examples

### Cron Scheduler

```c
#include "cap_cron.h"

// Recurring every hour
cron_add_expression("hourly_task", "@hourly", my_callback, NULL);

// Every 5 minutes
cron_add_expression("frequent_task", "*/5 * * * *", my_callback, NULL);

// One-shot in 30 seconds
cron_add_once("delayed_task", 30000, my_callback, NULL);
```

### Device Pairing

```c
#include "cap_pairing.h"

// Generate pairing code for Discord
char code[9];
pairing_generate_code(CHANNEL_DISCORD, code);
// Code format: "A7K9-M2XQ"

// Accept pairing
pairing_accept(code, user_id);
```

### Secrets Vault

```c
#include "cap_secrets.h"

// Store API key
secrets_set("openai_key", "sk-...", SECRET_TYPE_API_KEY);

// Retrieve (auto-decrypted)
char* key = secrets_get("openai_key");
```

### ReAct Agent

```c
#include "cap_react.h"

// Register a tool
react_register_tool("get_weather", weather_callback);

// Execute reasoning loop
react_reason("What's the weather in Tokyo?", response_buffer);
```

---

## 🛡️ Security Features

- ✅ **AES-256-GCM Encryption** for secrets vault
- ✅ **Secure Boot** support (enable in menuconfig)
- ✅ **Flash Encryption** support
- ✅ **HMAC Peripheral** for hardware crypto
- ✅ **Rate Limiting** to prevent abuse
- ✅ **Pairing Codes** with TTL expiry

---

## 🤝 Contributing

1. Fork the repository
2. Create feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open Pull Request

---

## 📄 License

Apache License 2.0 - See [LICENSE](LICENSE) file

---

## 🙏 Acknowledgments

Built with inspiration from:
- [ESP-Claw](https://github.com/espressif/esp-claw) by Espressif
- [OpenClaw](https://github.com/OpenClaw/OpenClaw)
- [zclaw](https://github.com/tnm/zclaw)
- [mimiclaw](https://github.com/memovai/mimiclaw)
- [femtoclaw](https://github.com/manjunathshiva/femtoclaw)
- [zeroclaw](https://github.com/zeroclaw-labs/zeroclaw)

---

## 📬 Support

- **Issues:** [GitHub Issues](https://github.com/maruf009sultan/neon-claw/issues)
- **Discussions:** [GitHub Discussions](https://github.com/maruf009sultan/neon-claw/discussions)
- **Email:** maruf009sultan@github.com

---

**Made with ❤️ by [maruf009sultan](https://github.com/maruf009sultan)**
