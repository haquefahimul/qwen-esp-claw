idf_component_register(
    SRCS "src/cap_secrets_vault.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer freertos nvs_flash spi_flash esp_http_client esp_https_client
)
