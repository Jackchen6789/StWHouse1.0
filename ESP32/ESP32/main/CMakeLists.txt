idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES mqtt esp_driver_uart driver esp_wifi esp_netif esp_event nvs_flash)