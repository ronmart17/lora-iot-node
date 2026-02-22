idf_component_register(
    SRCS
        "lora_driver.c"
        "pir_driver.c"
        "power_driver.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_adc esp_timer esp_wifi esp_hw_support
)