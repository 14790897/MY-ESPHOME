idf_component_register(
    SRCS "main.c"
    PRIV_REQUIRES
        openthread
        esp_coex
        esp_wifi
        nvs_flash
        esp_psram
        esp_http_server
    INCLUDE_DIRS "."
)
