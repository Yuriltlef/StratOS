cmake_minimum_required(VERSION 3.15)

project(MUSSTL
    VERSION 0.1.0
    DESCRIPTION "A modern C++ STL implementation for embedded systems"
    LANGUAGES CXX)

set(LOG_PROJECT_NAME "${PROJECT_NAME}")
set(LOG_ENABLE_COLORS FALSE CACHE BOOL "Enable coloreddebug logs" FORCE)

include(${PROJECT_SOURCE_DIR}/cmake/config.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/logUtils.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/initTest.cmake)

# ====================================================================
# 初始化消息
# ====================================================================


# 显示初始化信息
log_info("Log utilities loaded for project: ${LOG_PROJECT_NAME}")
log_info("Time format: ${LOG_TIME_FORMAT}")
if(LOG_ENABLE_COLORS)
    log_info("Colored output: ENABLED")
else()
    log_info("Colored output: DISABLED")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
log_info("C++ standard set to C++17")
log_info("C++ extensions disabled")

# 全局编译选项
if(MUSSTL_NO_EXCEPTIONS)
    add_compile_options(-fno-exceptions)
    log_info("Exception handling disabled: -fno-exceptions")
endif()

if(MUSSTL_NO_RTTI)
    add_compile_options(-fno-rtti)
    log_info("RTTI disabled: -fno-rtti")
endif()

# 优化选项（嵌入式系统通常需要-Os优化大小）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    log_info("No build type specified. Defaulting to Release.")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-Os)  # 优化代码大小
    add_compile_options(-ffunction-sections -fdata-sections)
    add_link_options(-Wl,--gc-sections)
    log_info("Release build: Size optimizations enabled: -Os")
    log_info("Function and data sections enabled for dead code elimination: -ffunction-sections -fdata-sections")
    log_info("Linker garbage collection enabled: -Wl,--gc-sections")
endif()

# ============================================
# 创建库目标
# ============================================
add_library(MUSSTL INTERFACE)  # 纯头文件库使用INTERFACE
log_info("Created INTERFACE library target: MUSSTL")

# 为不同构建类型配置别名
add_library(MUSSTL::MUSSTL ALIAS MUSSTL)

# 设置库属性
set_target_properties(MUSSTL PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(MUSSTL
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
log_info("Include directories set for MUSSTL target: ${CMAKE_CURRENT_SOURCE_DIR}/include")

# ============================================
# 编译定义
# ============================================
target_compile_definitions(MUSSTL INTERFACE
    MUSSTL_NO_DYNAMIC_ALLOCATION=1
    $<$<BOOL:${MUSSTL_NO_EXCEPTIONS}>:MUSSTL_NO_EXCEPTIONS=1>
    $<$<BOOL:${MUSSTL_NO_RTTI}>:MUSSTL_NO_RTTI=1>
    $<$<BOOL:${MUSSTL_STATIC_ALLOCATION}>:MUSSTL_STATIC_ALLOCATION=1>
)

# ============================================
# 导出目标配置
# ============================================
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# 生成导出头文件
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/MUSSTLConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/MUSSTLConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MUSSTL
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

# 生成版本文件
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/MUSSTLConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

# ============================================
# 安装配置
# ============================================
if(MUSSTL_INSTALL)
    # 设置自定义安装前缀
    if(MUSSTL_USER_INSTALL_PREFIX)
        # 如果用户没有指定安装前缀，则使用项目目录下的 install 文件夹
        set(CMAKE_INSTALL_PREFIX "${MUSSTL_USER_INSTALL_PREFIX}" CACHE PATH "Installation directory" FORCE)
    else()
        set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install" CACHE PATH "Installation directory" FORCE)
    endif()
    log_info("Installation prefix set to: ${CMAKE_INSTALL_PREFIX}")

    install(
        TARGETS MUSSTL
        EXPORT MUSSTLTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    
    # 安装头文件
    install(
        DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp"
    )
    
    # 安装导出文件
    install(
        EXPORT MUSSTLTargets
        FILE MUSSTLTargets.cmake
        NAMESPACE MUSSTL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MUSSTL
    )
    
    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/MUSSTLConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/MUSSTLConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MUSSTL
    )
endif()

# ============================================
# 测试初始化
# ============================================
init_test(BUILD_TESTS)
