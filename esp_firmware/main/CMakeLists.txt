idf_component_register(SRCS "matrix_driver.cpp" "http_server.c" "wifi_manager.c" "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi esp_http_server nvs_flash esp_netif espressif__arduino-esp32 espressif__mdns)

idf_build_set_property(EXCLUDE_COMPONENTS esp_lcd)

target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=esp_log_write")
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=esp_log_writev")
