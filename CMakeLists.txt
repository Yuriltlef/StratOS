cmake_minimum_required(VERSION 3.25)

#限制版本范围
if(${CMAKE_VERSION} VERSION_LESS 3.25)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# 导入配置
include(${CMAKE_SOURCE_DIR}/cmake/os_file_path.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/os_confing.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/os_util.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/path.cmake)

# 配置基础设置
set(LIYSTD_LICENSE "LGPL-3.0" CACHE STRING "Library license type")
# clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# make
set(CMAKE_VERBOSE_MAKEFILE ON)
# 彩色输出
set(CMAKE_COLOR_DIAGNOSTICS ON)
# 目标平台系统名称：裸机嵌入式 Generic
set(CMAKE_SYSTEM_NAME Generic)
# 目标平台的体系结构
set(CMAKE_SYSTEM_PROCESSOR arm)

# 编译器
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
# 跳过编译器 -rdynamic 检查
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

#项目描述
project(
    ${OS_PROJECT_NAME}
    VERSION ${OS_VERSION}
    DESCRIPTION ${OS_DESCRIPTION}
)
# 设置语言标准
enable_language(C CXX ASM)
set(CMAKE_C_STANDARD ${OS_C_STANDARD})
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD ${OS_CPP_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 输出基础配置信息
print_project_info()
message("[${CMAKE_PROJECT_NAME}][info] Now cmake version is " ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
message("[${CMAKE_PROJECT_NAME}][info] CXX standard of " ${PROJECT_NAME} " is " ${OS_CPP_STANDARD})
message("[${CMAKE_PROJECT_NAME}][info] C standard of " ${PROJECT_NAME} " is " ${OS_C_STANDARD})
message("[${CMAKE_PROJECT_NAME}][info] Color output is enabled")

configure_mutual_marco(${OS_STM_LIB_TYPE_FLAG} ${OS_STM_MODEL} ${OS_CONFING})

# 配置clangd
generate_clang_config(TOOLCHAIN_PATH ${ARM_TOOLCHAIN_PATH})

# 配置StratOS构建测试
add_executable(${OS_ELF_TARGET}
    ${OS_STARTUP}
)

configure_target(${OS_ELF_TARGET})

# 创建接口库
add_library(OS_INTERNAL_INC INTERFACE)
target_include_directories(OS_INTERNAL_INC INTERFACE 
    ${OS_CMSIS_INC}
    ${OS_STDPERIPH_DRIVER_INC}
)

# 对象库
add_library(OS_INTERNAL_SRC OBJECT 
    ${OS_STARTUP}
    ${OS_CMSIS_SRC}
    ${OS_STDPERIPH_DRIVER_SRC}
)
target_include_directories(OS_INTERNAL_SRC PRIVATE 
    ${OS_CMSIS_INC}
    ${OS_STDPERIPH_DRIVER_INC}
)

configure_target(OS_INTERNAL_SRC)

disable_cmsis_warnings()

add_subdirectory(${CMAKE_SOURCE_DIR}/user)
