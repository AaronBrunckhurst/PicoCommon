set(PICO_BOARD pico_w)

add_library(serial_console 
${CMAKE_CURRENT_LIST_DIR}/serial_console.cpp 
${CMAKE_CURRENT_LIST_DIR}/system_status.c)

target_link_libraries(serial_console
    pico_stdlib
    pico_time
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    serial_console
)