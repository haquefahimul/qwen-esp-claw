# Pairing Capability Component

cmake_minimum_required(VERSION 3.16)

idf_component_register(
    SRCS
        "src/cap_pairing.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        cJSON
        claw_cap
        claw_event_router
        freertos
)
