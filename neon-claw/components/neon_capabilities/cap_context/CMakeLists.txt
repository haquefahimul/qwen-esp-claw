idf_component_register(
    SRCS "src/cap_context.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer nvs_flash esp_http_client
)
