idf_component_register(SRCS 
                    "main.c"
                    "espnow/send.c"
                    "led/led.c"
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
