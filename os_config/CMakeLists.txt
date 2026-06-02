cmake_minimum_required(VERSION 3.25)

include(${CMAKE_SOURCE_DIR}/cmake/log_utils.cmake)
include(${CMAKE_SOURCE_DIR}/os_config/board/st/stm32f1/stm32f103MD.cmake)

# 生成链接脚本
log_info("Generating linker script...")
configure_file(
    ${CMAKE_SOURCE_DIR}/os_linker/template/stm32_f10x/STM32F103md_static_in.ld
    ${CMAKE_SOURCE_DIR}/os_linker/stm32_f10x/STM32F103md_static.ld
    @ONLY
)
log_info("Linker script generated at ${CMAKE_SOURCE_DIR}/os_linker/stm32_f10x/STM32F103md_static.ld")

# 生成配置头文件
log_info("Generating memory layout header...")
configure_file(
    ${CMAKE_SOURCE_DIR}/os_config/kernel/memory_layout.hpp.in
    ${CMAKE_SOURCE_DIR}/os_config/include/memory_layout.hpp
    @ONLY
)
log_info("Memory layout header generated at ${CMAKE_SOURCE_DIR}/os_config/include/memory_layout.hpp")
