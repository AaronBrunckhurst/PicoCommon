# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(common_example_pico C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
cmake_policy(SET CMP0079 NEW)
add_compile_options(-Wall -Wextra)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

if(PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME}
    main.cpp
)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

SET(BASEPATH "${CMAKE_SOURCE_DIR}")
INCLUDE_DIRECTORIES("${BASEPATH}")

add_subdirectory(serial_console)
add_subdirectory(filesystem)
add_subdirectory(ap_website)
add_subdirectory(http_server)
add_subdirectory(system)

# Linking Pico libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
    pico_stdlib
    pico_platform
    pico_multicore
    pico_time
    pico_cyw43_arch_none
    serial_console
    filesystem
    ap_website
    http_server
    system
)

# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
