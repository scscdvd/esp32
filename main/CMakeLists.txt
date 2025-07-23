set(SOURCES 
    "main.c"
    "../src/mywifi.c"
    "../src/mynvs.c"
    "../src/uart.c"
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "."
                 "../include"
)
