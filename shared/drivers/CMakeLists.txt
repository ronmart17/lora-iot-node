idf_component_register(
    SRCS
        "oled_driver.c"
    INCLUDE_DIRS "."
    NAME oled
    REQUIRES driver esp_timer
)