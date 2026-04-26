idf_component_register(
    SRCS "src/cap_secrets.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer nvs_flash esp_http_client
)
