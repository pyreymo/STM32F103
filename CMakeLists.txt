cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME stm32-dev)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Core/Src/face.c
    Lib/DHT11/DHT11.c
    Lib/SSD1306/u8g2_stm32_hal.c
    Lib/SSD1306/U8g2_csrc/u8g2_setup.c
    Lib/SSD1306/U8g2_csrc/u8g2_buffer.c
    Lib/SSD1306/U8g2_csrc/u8g2_font.c
    Lib/SSD1306/U8g2_csrc/u8g2_fonts.c
    Lib/SSD1306/U8g2_csrc/u8g2_line.c
    Lib/SSD1306/U8g2_csrc/u8g2_box.c
    Lib/SSD1306/U8g2_csrc/u8g2_circle.c
    Lib/SSD1306/U8g2_csrc/u8g2_arc.c
    Lib/SSD1306/U8g2_csrc/u8g2_polygon.c
    Lib/SSD1306/U8g2_csrc/u8g2_bitmap.c
    Lib/SSD1306/U8g2_csrc/u8g2_button.c
    Lib/SSD1306/U8g2_csrc/u8g2_cleardisplay.c
    Lib/SSD1306/U8g2_csrc/u8g2_d_memory.c
    Lib/SSD1306/U8g2_csrc/u8g2_d_setup.c
    Lib/SSD1306/U8g2_csrc/u8g2_hvline.c
    Lib/SSD1306/U8g2_csrc/u8g2_intersection.c
    Lib/SSD1306/U8g2_csrc/u8g2_kerning.c
    Lib/SSD1306/U8g2_csrc/u8g2_ll_hvline.c
    Lib/SSD1306/U8g2_csrc/u8x8_8x8.c
    Lib/SSD1306/U8g2_csrc/u8x8_byte.c
    Lib/SSD1306/U8g2_csrc/u8x8_cad.c
    Lib/SSD1306/U8g2_csrc/u8x8_display.c
    Lib/SSD1306/U8g2_csrc/u8x8_fonts.c
    Lib/SSD1306/U8g2_csrc/u8x8_gpio.c
    Lib/SSD1306/U8g2_csrc/u8x8_setup.c
    Lib/SSD1306/U8g2_csrc/u8x8_string.c
    Lib/SSD1306/U8g2_csrc/u8x8_u8toa.c
    Lib/SSD1306/U8g2_csrc/u8x8_u16toa.c
    Lib/SSD1306/U8g2_csrc/u8x8_selection_list.c
    Lib/SSD1306/U8g2_csrc/u8x8_input_value.c
    Lib/SSD1306/U8g2_csrc/u8x8_message.c
    Lib/SSD1306/U8g2_csrc/u8x8_debounce.c
    Lib/SSD1306/U8g2_csrc/u8x8_capture.c
    Lib/SSD1306/U8g2_csrc/u8x8_d_ssd1306_128x64_noname.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Lib/DHT11
    Lib/SSD1306
    Lib/SSD1306/U8g2_csrc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
