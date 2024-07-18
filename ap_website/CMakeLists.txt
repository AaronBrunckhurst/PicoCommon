# Set minimum required version of CMake

set(PICO_BOARD pico_w)

add_library(ap_website
    ${CMAKE_CURRENT_LIST_DIR}/dhcpserver/dhcpserver.c 
    ${CMAKE_CURRENT_LIST_DIR}/dnsserver/dnsserver.c
    ${CMAKE_CURRENT_LIST_DIR}/picow_access_point.c
    ${CMAKE_CURRENT_LIST_DIR}/url_collection.cpp
)

target_include_directories(ap_website PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/dnsserver
        ${CMAKE_CURRENT_LIST_DIR}/
        )

SET(BASEPATH "${CMAKE_SOURCE_DIR}")
INCLUDE_DIRECTORIES("${BASEPATH}")

target_link_libraries(ap_website
    pico_stdlib
    pico_platform
    pico_multicore
    pico_time
    pico_cyw43_arch_lwip_threadsafe_background
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    ap_website
)