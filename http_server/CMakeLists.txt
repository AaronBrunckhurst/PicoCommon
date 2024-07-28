# Set minimum required version of CMake

set(PICO_BOARD pico_w)

add_library(http_server
    ${CMAKE_CURRENT_LIST_DIR}/wifi.c
    ${CMAKE_CURRENT_LIST_DIR}/tcp_server.c
    ${CMAKE_CURRENT_LIST_DIR}/http_server.c
    ${CMAKE_CURRENT_LIST_DIR}/url_collection.cpp
)

SET(BASEPATH "${CMAKE_SOURCE_DIR}")
INCLUDE_DIRECTORIES("${BASEPATH}")

target_link_libraries(http_server
    pico_stdlib
    pico_platform
    pico_multicore
    pico_time
    pico_cyw43_arch_lwip_threadsafe_background
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    http_server
)