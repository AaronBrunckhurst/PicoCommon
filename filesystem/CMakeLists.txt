add_library(filesystem ${CMAKE_CURRENT_LIST_DIR}/filesystem.c ${CMAKE_CURRENT_LIST_DIR}/filesystem.h)

add_subdirectory(../libraries/pico_flash_littlefs build)

target_link_libraries(filesystem
    pico_stdlib
    pico_time
    littlefs-lib
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    filesystem
)