cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(pico_sdk_import.cmake)

project(keypad C CXX ASM)

pico_sdk_init()

add_executable(keypad src/main.cpp src/key.cpp src/usb_descriptors.c src/usb_stuff.cpp)
target_link_libraries(keypad PUBLIC pico_unique_id pico_stdlib hardware_adc tinyusb_device tinyusb_board)
target_include_directories(keypad PUBLIC src/)

pico_enable_stdio_usb(keypad 0)
pico_add_extra_outputs(keypad)
