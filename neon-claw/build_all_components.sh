#!/bin/bash
echo "=== NEON-CLAW: Building All Components ==="

# Create all component directories
cd /workspace/neon-claw/components/neon_capabilities

for comp in cap_hooks cap_heartbeat cap_pairing cap_secrets cap_ratelimit cap_react cap_context cap_vector cap_eventbus cap_watchdog cap_prompt; do
    mkdir -p $comp/{include,src}
done

echo "✓ Directory structure created"

# Generate CMakeLists for all
for comp in cap_hooks cap_heartbeat cap_pairing cap_secrets cap_ratelimit cap_react cap_context cap_vector cap_eventbus cap_watchdog cap_prompt; do
    cat > $comp/CMakeLists.txt << EOF
idf_component_register(
    SRCS "src/$comp.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer nvs_flash esp_http_client
)
EOF
done

echo "✓ CMakeLists.txt files created"
