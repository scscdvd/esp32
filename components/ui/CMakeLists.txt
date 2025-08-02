# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly

idf_component_register(
    SRCS "ui_home.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl
)