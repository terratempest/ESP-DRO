# main/CMakeLists.txt

# Recursively find all .c and .cpp files in 'main/' and its subdirectories
file(GLOB_RECURSE MAIN_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Recursively find all .h and .hpp files
file(GLOB_RECURSE MAIN_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

# Collect all unique directories containing headers
set(INCLUDE_DIRS ".")
foreach(header ${MAIN_HEADERS})
    get_filename_component(dir ${header} DIRECTORY)
    list(APPEND INCLUDE_DIRS ${dir})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)

# Register this component with ESP-IDF (also depends on lvgl and touch components)
idf_component_register(
    SRCS ${MAIN_SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES lvgl esp_lcd_touch_gt911 nvs_flash
)
