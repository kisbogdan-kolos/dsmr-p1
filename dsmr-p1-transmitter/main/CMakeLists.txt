idf_component_register(SRCS 
                    "main.c"
                    "telegram/processData.c"
                    "telegram/telegram.c"
                    "espnow/send.c"
                    "spiffs/datastore.c"
                    "uart/uart.c"
                    "led/led.c"
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
