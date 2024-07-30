# Set minimum required version of CMake

add_library(system
    ${CMAKE_CURRENT_LIST_DIR}/temperture.c
)

SET(BASEPATH "${CMAKE_SOURCE_DIR}")
INCLUDE_DIRECTORIES("${BASEPATH}")

target_link_libraries(system
    pico_stdlib
    pico_platform
    pico_multicore
    pico_time
    hardware_adc
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    system
)