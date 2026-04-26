idf_component_register(
    SRCS "src/cap_ota.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_client app_update
)
