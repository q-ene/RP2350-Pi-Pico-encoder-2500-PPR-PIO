cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(Pico2_encoder_PIO C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Генерация заголовка из PIO-программы — обязательный шаг
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/encoder.pio)

add_executable(${PROJECT_NAME}
    main.c
    encoder.c
    lcd_i2c.c
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_stdio_usb
    hardware_pio
    hardware_dma
    hardware_i2c
    hardware_gpio
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})
