set(SOURCES 
    "main.c"
    "../src/mywifi.c"
    "../src/mynvs.c"
    "../src/uart.c"
    "../src/io_control.c"
    "../src/WS2812.c"
    "../src/data_analysis.c"
    "../src/aliot_ota.c"
    "../src/aliot_dm.c"
    )

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "."
                 "../include"
)
