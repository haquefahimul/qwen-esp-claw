# Secrets Vault Capability Component

cmake_minimum_required(VERSION 3.16)

idf_component_register(
    SRCS
        "src/cap_secrets.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        cJSON
        claw_cap
        freertos
        esp_crypto
)
