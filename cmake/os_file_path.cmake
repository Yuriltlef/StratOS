# ====================== 链接脚本 ======================
set(OS_LD_SCRIPT 
    "${CMAKE_SOURCE_DIR}/link/stm32_f10x/STM32F103md_FLASH.ld"
)

# ====================== 启动文件 ======================
set(OS_STARTUP
    "${CMAKE_SOURCE_DIR}/boost/stm32_f10x/startup_stm32f10x_md.s"
)

# ====================== CMSIS 内核文件 ======================
file(GLOB_RECURSE OS_CMSIS_SRC
    "${CMAKE_SOURCE_DIR}/stm32SL_temp/Libraries/CMSIS/CM3/CoreSupport/*.c"
    "${CMAKE_SOURCE_DIR}/stm32SL_temp/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/*.c"
)
set(OS_CMSIS_INC
    "${CMAKE_SOURCE_DIR}/stm32SL_temp/Libraries/CMSIS/"
    "${CMAKE_SOURCE_DIR}/stm32SL_temp/Libraries/CMSIS/CM3/CoreSupport/"
    "${CMAKE_SOURCE_DIR}/stm32SL_temp/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/"
)

# ====================== 标准库外设源文件 ======================
file(GLOB_RECURSE OS_STDPERIPH_DRIVER_SRC
    "${CMAKE_SOURCE_DIR}/stm32SL_temp/Libraries/STM32F10x_StdPeriph_Driver/src/*.c"
    "${CMAKE_SOURCE_DIR}/stm32SL_temp/Libraries/STM32F10x_StdPeriph_Driver/src/*.cpp"
)
set(OS_STDPERIPH_DRIVER_INC
    "${CMAKE_SOURCE_DIR}/stm32SL_temp/Libraries/"
    "${CMAKE_SOURCE_DIR}/stm32SL_temp/Libraries/STM32F10x_StdPeriph_Driver/"
    "${CMAKE_SOURCE_DIR}/stm32SL_temp/Libraries/STM32F10x_StdPeriph_Driver/inc/"
)

# ====================== 用户文件 ======================
file(GLOB_RECURSE USER_SRC
    "${CMAKE_SOURCE_DIR}/user/libraries/*.c"
    "${CMAKE_SOURCE_DIR}/user/libraries/*.cpp"
    "${CMAKE_SOURCE_DIR}/user/src/*.c"
    "${CMAKE_SOURCE_DIR}/user/src/*.cpp"
)
set(USER_INC
    "${CMAKE_SOURCE_DIR}/user/libraries/"
    "${CMAKE_SOURCE_DIR}/user/inc/"
)

# ====================== 二进制输出目录 ======================
set(PROJECT_OUTPUT_DIR
    "${CMAKE_SOURCE_DIR}/bin/"
)