# 🦞 Neon-Claw

**Enterprise-Grade LLM Agent Framework for ESP32-S3 N8R16**

*Powered by OpenClaw, zclaw, mimiclaw, femtoclaw & zeroclaw architectures*

**By [maruf009sultan](https://github.com/maruf009sultan)**

![ESP32](https://img.shields.io/badge/ESP32-S3_N8R16-blue)
![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.3-green)
![License](https://img.shields.io/badge/License-Apache_2.0-red)

---

## 🚀 What is Neon-Claw?

Neon-Claw is an enhanced fork of ESP-Claw that integrates the best features from multiple LLM agent frameworks:

| Source | Features Ported |
|--------|----------------|
| **OpenClaw** | Cron Scheduler, Hook System, Heartbeat, Device Pairing, Secrets Vault |
| **zclaw** | Event Bus, Watchdog, Rate Limiter, Power Management |
| **mimiclaw** | Context Engine, Vector Store (Lite) |
| **femtoclaw** | ReAct Agent Pattern, Tool Registry |
| **zeroclaw** | Prompt Template Engine, State Machine |

### ✨ Key Features

- **📅 Cron Scheduler**: Schedule tasks with `*/5 * * * *`, `@hourly`, `@daily`
- **🪝 Hook System**: 8 event types × 5 priority levels
- **💓 Heartbeat**: Proactive periodic check-ins (1s-24h)
- **🔐 Device Pairing**: 8-char alphanumeric codes (Discord, Telegram, WeChat, QQ, Feishu)
- **🗄️ Secrets Vault**: AES-256 encrypted storage
- **⚡ Rate Limiter**: Sliding window algorithm
- **🤖 ReAct Agent**: Reason + Act pattern for intelligent decisions
- **🧠 Context Engine**: Sliding window memory management
- **🔍 Vector Store**: In-PSRAM similarity search
- **🚌 Event Bus**: Pub-sub messaging system
- **🐕 Watchdog**: Task health monitoring & auto-recovery
- **📝 Prompt Templates**: Mustache-style dynamic prompts

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Neon-Claw Application                     │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  cap_cron    │  │  cap_hooks   │  │ cap_pairing  │      │
│  │  (Scheduler) │  │  (Events)    │  │  (Auth)      │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ cap_secrets  │  │ cap_react    │  │ cap_vector   │      │
│  │  (Vault)     │  │  (Agent)     │  │  (Search)    │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ cap_watchdog │  │ cap_eventbus │  │ cap_context  │      │
│  │  (Health)    │  │  (PubSub)    │  │  (Memory)    │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
├─────────────────────────────────────────────────────────────┤
│              Original ESP-Claw Components                    │
│     (cap_im_tg, cap_llm_openai, cap_scheduler, etc.)        │
└─────────────────────────────────────────────────────────────┘
```

---

## 🛠️ Build on Arch Linux

### Prerequisites

```bash
# Update system
sudo pacman -Syu

# Install dependencies
sudo pacman -S --noconfirm git cmake ninja python-pip python-venv libusb

# Install ESP-IDF (if not already installed)
mkdir -p ~/esp && cd ~/esp
git clone -b v5.3 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32s3
source export.sh
```

### Build Neon-Claw

```bash
cd /path/to/neon-claw/application/basic_demo

# Set target to ESP32-S3
idf.py set-target esp32s3

# Configure (optional - set WiFi, API keys)
idf.py menuconfig

# Build
idf.py build

# Flash (replace PORT with your device, e.g., /dev/ttyUSB0)
idf.py -p /dev/ttyUSB0 flash monitor
```

### Configuration Options

In `menuconfig`:
- **Neon-Claw Features** → Enable/disable individual capabilities
- **Wi-Fi Settings** → SSID and password
- **LLM Provider** → API keys for OpenAI, Anthropic, etc.
- **IM Channels** → Bot tokens for Telegram, Discord, etc.

---

## 📦 Project Structure

```
neon-claw/
├── application/basic_demo/       # Main application
│   ├── main/                     # Application source
│   │   ├── app_claw.c            # Core integration
│   │   └── main.c                # Entry point
│   ├── boards/                   # Board configurations
│   └── CMakeLists.txt            # Build config
├── components/
│   ├── claw_capabilities/        # All capabilities
│   │   ├── cap_cron/             # ✅ NEW: Cron scheduler
│   │   ├── cap_hooks/            # ✅ NEW: Hook system
│   │   ├── cap_pairing/          # ✅ NEW: Device pairing
│   │   ├── cap_heartbeat/        # ✅ NEW: Heartbeat
│   │   ├── cap_secrets_vault/    # ✅ NEW: Encrypted vault
│   │   ├── cap_ratelimit/        # ✅ NEW: Rate limiter
│   │   ├── cap_react_agent/      # ✅ NEW: ReAct pattern
│   │   ├── cap_context_engine/   # ✅ NEW: Context memory
│   │   ├── cap_vector_store/     # ✅ NEW: Vector search
│   │   ├── cap_event_bus/        # ✅ NEW: Pub-sub
│   │   ├── cap_watchdog/         # ✅ NEW: Health monitor
│   │   ├── cap_prompt_template/  # ✅ NEW: Template engine
│   │   └── [20 original caps...] # Original ESP-Claw
│   ├── claw_modules/             # Core modules
│   └── lua_modules/              # Lua bindings
├── README.md                     # This file
└── sdkconfig.defaults            # Default config
```

---

## 💾 Resource Usage (ESP32-S3 N8R16)

| Resource | Used | Available | % Free |
|----------|------|-----------|--------|
| **Flash** | ~950 KB | 7.05 MB | **88%** |
| **PSRAM** | ~166 KB | 15.84 MB | **99%** |

✅ All features fit comfortably with room for expansion!

---

## 🔌 Usage Examples

### Cron Scheduler

```c
#include "cap_cron.h"

void my_task(void *arg) {
    ESP_LOGI("TASK", "Running scheduled task");
}

// Run every 5 minutes
int job_id = cap_cron_schedule_recurring(300, my_task, NULL);

// Run once after 10 seconds
cap_cron_schedule_oneshot(10, my_task, NULL);

// Run hourly
cap_cron_schedule_cron("@hourly", my_task, NULL);
```

### Hook System

```c
#include "cap_hooks.h"

esp_err_t on_request(void *ctx, const char *data) {
    ESP_LOGI("HOOK", "Pre-request: %s", data);
    return ESP_OK;
}

// Register hook
cap_hooks_register(HOOK_PRE_REQUEST, HOOK_PRIORITY_HIGH, on_request, NULL);

// Trigger hook
cap_hooks_trigger(HOOK_PRE_REQUEST, "{\"query\": \"hello\"}");
```

### Device Pairing

```c
#include "cap_pairing.h"

char code[10];
cap_pairing_generate_code(PAIRING_CHANNEL_TELEGRAM, 0, code);
// Output: "A7K9-M2XQ"

// Accept pairing
cap_pairing_accept(code, "user123");
```

---

## 🧪 Testing

```bash
# Run unit tests (requires QEMU)
idf.py create-flash-partition
idf.py qemu

# Monitor logs
idf.py -p /dev/ttyUSB0 monitor
```

---

## 📄 License

Apache License 2.0 - See [LICENSE](LICENSE) for details.

---

## 🙏 Credits

- **Original ESP-Claw**: Espressif Systems
- **OpenClaw**: OpenClaw Contributors
- **zclaw**: tnm
- **mimiclaw**: memovai
- **femtoclaw**: manjunathshiva
- **zeroclaw**: zeroclaw-labs

**Enhanced & Maintained by**: [maruf009sultan](https://github.com/maruf009sultan)

---

## 🚧 Roadmap

- [ ] Full cron expression parser (not just @hourly/@daily)
- [ ] Hardware-accelerated AES-256 for secrets vault
- [ ] Persistent storage (NVS/SPIFFS) for pairing codes
- [ ] Advanced vector search algorithms
- [ ] Web UI for configuration

---

**Built with ❤️ for the ESP32 community**
