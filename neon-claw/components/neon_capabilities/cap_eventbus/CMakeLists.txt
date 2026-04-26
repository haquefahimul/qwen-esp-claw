idf_component_register(
    SRCS "src/cap_eventbus.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer nvs_flash esp_http_client
)
