cmake_minimum_required(VERSION 3.22)

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
set(CMAKE_PROJECT_NAME Project1_VscodeLed)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources (Drivers, HAL, Middlewares, etc.)
add_subdirectory(cmake/stm32cubemx)

# ==============================================================================
# [用户配置区] 添加你的自定义驱动路径
# ==============================================================================

# 1. 包含头文件路径 (Include Paths)
# --------------------------------------------------------
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # CubeMX 默认生成的
    "Core/Inc"
    
    # 驱动文件夹
    "led_driver2"
    "tf_driver"
    "lcd_driver"
    "led_driver1"
    
    # 如果以后加了别的，继续在这里追加，例如：
    # "oled_driver"
    # "sd_driver"
)

# 2. 添加源文件 (Source Files)
# --------------------------------------------------------
# 使用 GLOB_RECURSE 自动扫描文件夹下的 .c 文件
file(GLOB_RECURSE USER_DRIVER_SOURCES
    "led_driver2/*.c"
    "tf_driver/*.c"
    "lcd_driver/*.c"
    "led_driver1/*.c"
    
    # 注意：Core/Src 已经在 stm32cubemx 子目录里被添加过了
    # 这里只加你自己新建的驱动文件夹
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${USER_DRIVER_SOURCES}
)

# ==============================================================================
# [编译器链接选项] 浮点打印 & Nano库
# ==============================================================================
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE 
    # 生成 map 文件 (方便查看内存占用)
    -Wl,-Map=${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map,--cref
    
    # 1. 启用 Newlib-nano (GCC版的 MicroLIB，减小体积)
    #--specs=nano.specs
    
    # 2. 解决 printf 浮点数无法打印的问题 (启用 %f 支持)
    -u _printf_float
)

# ==============================================================================

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
)