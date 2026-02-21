idf_component_register(
    SRCS
        "lora_service.c"
        "display_service.c"
    INCLUDE_DIRS "."
    REQUIRES drivers protocol oled
)