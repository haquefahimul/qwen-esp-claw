idf_component_register(SRCS "src/cap_cron.c"
                       INCLUDE_DIRS "include"
                       REQUIRES freertos esp_timer
                       PRIV_REQUIRES nvs_flash)
