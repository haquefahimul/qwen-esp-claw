cmake_minimum_required(VERSION 3.16)

idf_component_register(
    SRCS "src/cap_context.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos esp_http_client json
    PRIV_REQUIRES esp_heap_caps
)
