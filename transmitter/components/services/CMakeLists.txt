idf_component_register(
    SRCS
        "event_service.c"
        "lora_service.c"
        "power_manager.c"
        "display_service.c"
    INCLUDE_DIRS "."
    REQUIRES drivers protocol oled_driver esp_timer
)