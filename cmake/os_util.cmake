#
# ******************************************************************************
# * @file    os_util.cmake
# * @author  Yurilt
# * @version V1.0.0
# * @date    30-October-2025
# * @brief   CMake构建脚本
# * @note    此文件包含CMake构建系统的配置指令
# ******************************************************************************
# * @attention
# *
# * Copyright (c) 2025 Yurilt.
# * All rights reserved.
# *
# * This software is licensed under terms that can be found in the LICENSE file
# * in the root directory of this software component.
# * If no LICENSE file comes with this software, it is provided AS-IS.
# *
# ******************************************************************************
#

#
string(LENGTH "[${OS_PROJECT_NAME}][info] " log_head_len)
string(REPEAT " " ${log_head_len} log_head_padding)
# 基础宏定义
function(configure_mutual_marco)
    foreach(marco IN LISTS ARGN)
        add_definitions(-D${marco})
        message("[${CMAKE_PROJECT_NAME}][info] defined global marco: " ${marco})
    endforeach()
endfunction()
# 为特定目标设置编译选项
function(configure_arm_compiler target_name)
    target_compile_definitions(${target_name} PRIVATE
        $<$<COMPILE_LANGUAGE:C>:
            __CORTEX_M3
        >
        $<$<COMPILE_LANGUAGE:CXX>:
            __CPLUSPLUS
            NO_EXCEPTIONS
            NO_RTTI
        >
    )
    message("[${CMAKE_PROJECT_NAME}][info] ${target_name} defined C: __CORTEX_M3")
    message("[${CMAKE_PROJECT_NAME}][info] ${target_name} defined CPP: __CPLUSPLUS NO_EXCEPTIONS NO_RTTI")
endfunction()
# 输出提示:美观的项目名和版本号
function(print_project_info)
    # 获取当前日期
    string(TIMESTAMP CURRENT_DATE "%Y-%m-%d")
    string(TIMESTAMP CURRENT_TIME "%H:%M:%S")
    message("")
    message("[${CMAKE_PROJECT_NAME}][info]┌────────────────────────────────────────┐")
    message("[${CMAKE_PROJECT_NAME}][info]│            STRATOS  FIRMWARE           │")
    message("[${CMAKE_PROJECT_NAME}][info]├────────────────────────────────────────┤")
    message("[${CMAKE_PROJECT_NAME}][info]│  Project: ${CMAKE_PROJECT_NAME}")
    message("[${CMAKE_PROJECT_NAME}][info]│                                        │")
    message("[${CMAKE_PROJECT_NAME}][info]│  Version: ${PROJECT_VERSION}")
    message("[${CMAKE_PROJECT_NAME}][info]│                                        │")
    message("[${CMAKE_PROJECT_NAME}][info]│  Build:   ${CURRENT_DATE} ${CURRENT_TIME}")
    message("[${CMAKE_PROJECT_NAME}][info]│                                        │")
    message("[${CMAKE_PROJECT_NAME}][info]│  Target:  ${CMAKE_SYSTEM_PROCESSOR} ${OS_MCU}")
    message("[${CMAKE_PROJECT_NAME}][info]│                                        │")
    message("[${CMAKE_PROJECT_NAME}][info]│  Build Type:  ${CMAKE_BUILD_TYPE}")
    message("[${CMAKE_PROJECT_NAME}][info]└────────────────────────────────────────┘")
    message("")
endfunction()
# 配置目标编译指令
function(configure_compiler target_name)
    target_compile_options(${target_name} PRIVATE
        ${OS_MUTUAL_FLAGS}
    )
    target_compile_options(${target_name} PRIVATE
        $<$<CONFIG:Debug>:
            $<$<COMPILE_LANGUAGE:CXX>:
                ${OS_CPP_FLAGS}
                ${OS_CPP_WARNNING_FLAGS}
                ${OS_CPP_DEBUG_FLAGS}
            >
            $<$<COMPILE_LANGUAGE:C>:
                ${OS_C_FLAGS}
                ${OS_C_WARNNING_FLAGS}
                ${OS_C_DEBUG_FLAGS}
            >
            $<$<COMPILE_LANGUAGE:ASM>:
                ${OS_ASM_FLAGS}
            >
        >
        $<$<CONFIG:Release>:
            $<$<COMPILE_LANGUAGE:CXX>:
                ${OS_CPP_FLAGS}
                ${OS_CPP_WARNNING_FLAGS}
                ${OS_CPP_RELEASE_FLAGS}
            >
            $<$<COMPILE_LANGUAGE:C>:
                ${OS_C_FLAGS}
                ${OS_C_WARNNING_FLAGS}
                ${OS_C_RELEASE_FLAGS}
            >
            $<$<COMPILE_LANGUAGE:ASM>:
                ${OS_ASM_FLAGS}
            >
        >
        $<$<CONFIG:MinSizeRel>:
            $<$<COMPILE_LANGUAGE:CXX>:
                ${OS_CPP_FLAGS}
                ${OS_CPP_WARNNING_FLAGS}
                ${OS_CPP_MINSIZEREL_FLAGS}
            >
            $<$<COMPILE_LANGUAGE:C>:
                ${OS_C_FLAGS}
                ${OS_C_WARNNING_FLAGS}
                ${OS_C_MINSIZEREL_FLAGS}
            >
            $<$<COMPILE_LANGUAGE:ASM>:
                ${OS_ASM_FLAGS}
            >
        >
        $<$<CONFIG:RelWithDebInfo>:
            $<$<COMPILE_LANGUAGE:CXX>:
                ${OS_CPP_FLAGS}
                ${OS_CPP_WARNNING_FLAGS}
                ${OS_CPP_RELWITHDEBINFO_FLAGS}
            >
            $<$<COMPILE_LANGUAGE:C>:
                ${OS_C_FLAGS}
                ${OS_C_WARNNING_FLAGS}
                ${OS_C_RELWITHDEBINFO_FLAGS}
            >
            $<$<COMPILE_LANGUAGE:ASM>:
                ${OS_ASM_FLAGS}
            >
        >
    )
    # 转换为字符串用于输出
    list(JOIN OS_MUTUAL_FLAGS " " OS_MUTUAL_FLAGS_STR)
    list(JOIN OS_CPP_FLAGS " " OS_CPP_FLAGS_STR)
    list(JOIN OS_CPP_WARNNING_FLAGS " " OS_CPP_WARNNING_STR)
    list(JOIN OS_CPP_DEBUG_FLAGS " " OS_CPP_DEBUG_FLAGS_STR)
    list(JOIN OS_CPP_RELEASE_FLAGS " " OS_CPP_RELEASE_FLAGS_STR)
    list(JOIN OS_CPP_MINSIZEREL_FLAGS " " OS_CPP_MINSIZEREL_FLAGS_STR)
    list(JOIN OS_CPP_RELWITHDEBINFO_FLAGS " " OS_CPP_RELWITHDEBINFO_FLAGS_STR)
    list(JOIN OS_C_FLAGS " " OS_C_FLAGS_STR)
    list(JOIN OS_C_WARNNING_FLAGS " " OS_C_WARNNING_FLAGS_STR)
    list(JOIN OS_C_DEBUG_FLAGS " " OS_C_DEBUG_FLAGS_STR)
    list(JOIN OS_C_RELEASE_FLAGS " " OS_C_RELEASE_FLAGS_STR)
    list(JOIN OS_C_MINSIZEREL_FLAGS " " OS_C_MINSIZEREL_FLAGS_STR)
    list(JOIN OS_C_RELWITHDEBINFO_FLAGS " " OS_C_RELWITHDEBINFO_FLAGS_STR)
    list(JOIN OS_ASM_FLAGS " " OS_ASM_FLAGS_STR)
    if(ENABLE_FULL_COMPILE_OPTIONS)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add CPP ${CMAKE_BUILD_TYPE} compile options:
                ${OS_MUTUAL_FLAGS_STR}
                ${OS_CPP_FLAGS_STR}
                ${OS_CPP_WARNNING_STR}
                ${OS_CPP_DEBUG_FLAGS_STR}"
            )
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add C ${CMAKE_BUILD_TYPE} compile options:
                ${OS_MUTUAL_FLAGS_STR}
                ${OS_C_FLAGS_STR}
                ${OS_C_WARNNING_FLAGS_STR}
                ${OS_C_DEBUG_FLAGS_STR}"
            )
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add ASM ${CMAKE_BUILD_TYPE} compile options:
                ${OS_ASM_FLAGS_STR}"
            )
        elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add CPP ${CMAKE_BUILD_TYPE} compile options:
                ${OS_MUTUAL_FLAGS_STR}
                ${OS_CPP_FLAGS_STR}
                ${OS_CPP_WARNNING_STR}
                ${OS_CPP_RELEASE_FLAGS_STR}"
            )
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add C ${CMAKE_BUILD_TYPE} compile options:
                ${OS_MUTUAL_FLAGS_STR}
                ${OS_C_FLAGS_STR}
                ${OS_C_WARNNING_FLAGS_STR}
                ${OS_C_RELEASE_FLAGS_STR}"
            )
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add ASM ${CMAKE_BUILD_TYPE} compile options:
                ${OS_ASM_FLAGS_STR}"
            )
        elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add CPP ${CMAKE_BUILD_TYPE} compile options:
                ${OS_MUTUAL_FLAGS_STR}
                ${OS_CPP_FLAGS_STR}
                ${OS_CPP_WARNNING_STR}
                ${OS_CPP_MINSIZEREL_FLAGS_STR}"
            )
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add C ${CMAKE_BUILD_TYPE} compile options:
                ${OS_MUTUAL_FLAGS_STR}
                ${OS_C_FLAGS_STR}
                ${OS_C_WARNNING_FLAGS_STR}
                ${OS_C_MINSIZEREL_FLAGS_STR}"
            )
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add ASM ${CMAKE_BUILD_TYPE} compile options:
                ${OS_ASM_FLAGS_STR}"
            )
        elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add CPP ${CMAKE_BUILD_TYPE} compile options:
                ${OS_MUTUAL_FLAGS_STR}
                ${OS_CPP_FLAGS_STR}
                ${OS_CPP_WARNNING_STR}
                ${OS_CPP_RELWITHDEBINFO_FLAGS_STR}"
            )
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add C ${CMAKE_BUILD_TYPE} compile options:
                ${OS_MUTUAL_FLAGS_STR}
                ${OS_C_FLAGS_STR}
                ${OS_C_WARNNING_FLAGS_STR}
                ${OS_C_RELWITHDEBINFO_FLAGS_STR}"
            )
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add ASM ${CMAKE_BUILD_TYPE} compile options:
                ${OS_ASM_FLAGS_STR}"
            )
        endif()
    endif()
endfunction()
# 配置目标链接器选项（支持不同构建类型）
function(configure_linker target_name)
    set(_OS_MAP_OPTION "-Wl,-Map=${target_name}.map")
    # 应用链接器选项
    target_link_options(${target_name} PRIVATE
        $<$<CONFIG:Debug>:
            ${OS_LD_BASE_FLAGS}
            ${_OS_MAP_OPTION}
            ${OS_LD_DEBUG_FLAGS}
            $<$<COMPILE_LANGUAGE:CXX>:${OS_CPP_LD_FLAGS}>
        >
        $<$<CONFIG:Release>:
            ${OS_LD_BASE_FLAGS}
            ${_OS_MAP_OPTION}
            ${OS_LD_RELEASE_FLAGS}
            $<$<COMPILE_LANGUAGE:CXX>:${OS_CPP_LD_FLAGS}>
        >
        $<$<CONFIG:MinSizeRel>:
            ${OS_LD_BASE_FLAGS}
            ${_OS_MAP_OPTION}
            ${OS_LD_MINSIZEREL_FLAGS}
            $<$<COMPILE_LANGUAGE:CXX>:${OS_CPP_LD_FLAGS}>
        >
        $<$<CONFIG:RelWithDebInfo>:
            ${OS_LD_BASE_FLAGS}
            ${_OS_MAP_OPTION}
            ${OS_LD_RELWITHDEBINFO_FLAGS}
            $<$<COMPILE_LANGUAGE:CXX>:${OS_CPP_LD_FLAGS}>
        >
    )
    # 转换为字符串用于输出
    list(JOIN OS_LD_BASE_FLAGS " " OS_LD_BASE_FLAGS_STR)
    list(JOIN OS_CPP_LD_FLAGS " " OS_CPP_LD_FLAGS_STR)
    list(JOIN OS_LD_DEBUG_FLAGS " " OS_LD_DEBUG_FLAGS_STR)
    list(JOIN OS_LD_RELEASE_FLAGS " " OS_LD_RELEASE_FLAGS_STR)
    list(JOIN OS_LD_MINSIZEREL_FLAGS " " OS_LD_MINSIZEREL_FLAGS_STR)
    list(JOIN OS_LD_RELWITHDEBINFO_FLAGS " " OS_LD_RELWITHDEBINFO_FLAGS_STR)
    if(ENABLE_FULL_LD_OPTIONS)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add LD ${CMAKE_BUILD_TYPE} link options:
                ${OS_LD_BASE_FLAGS_STR}
                ${_OS_MAP_OPTION}
                ${OS_LD_DEBUG_FLAGS_STR}"
            )
        elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add LD ${CMAKE_BUILD_TYPE} link options:
                ${OS_LD_BASE_FLAGS_STR}
                ${_OS_MAP_OPTION}
                ${OS_LD_RELEASE_FLAGS_STR}"
            )
        elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add LD ${CMAKE_BUILD_TYPE} link options:
                ${OS_LD_BASE_FLAGS_STR}
                ${_OS_MAP_OPTION}
                ${OS_LD_MINSIZEREL_FLAGS_STR}"
            )
        elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            message("[${CMAKE_PROJECT_NAME}][info] ${target_name} add LD ${CMAKE_BUILD_TYPE} link options:
                ${OS_LD_BASE_FLAGS_STR}
                ${_OS_MAP_OPTION}
                ${OS_LD_RELWITHDEBINFO_FLAGS_STR}"
            )
        endif()
        if(OS_CPP_LD_FLAGS)
            # string(LENGTH "[${CMAKE_PROJECT_NAME}][info] " log_head_len)
            # string(REPEAT " " ${log_head_len} log_head_padding)
            message(
                "${log_head_padding}${OS_CPP_LD_FLAGS_STR}"
            )
        endif()
    endif()
endfunction()
#配置目标的所有命令
function(configure_target target_name)
    message("[${CMAKE_PROJECT_NAME}][info] ${target_name} build type: ${CMAKE_BUILD_TYPE}")
    configure_arm_compiler(${target_name})
    configure_compiler(${target_name})
    configure_linker(${target_name})
    if(ENABLE_LGE_CHECK)
        debug_language_detection(${target_name})
    endif()
endfunction()
# 配置目标输出文件并移动到指定目录
function(configure_target_output target_name output_dir)
    # 确保输出目录存在
    file(MAKE_DIRECTORY ${output_dir})
    set_target_properties(${target_name} PROPERTIES
        OUTPUT_NAME "${target_name}"
        SUFFIX ".elf"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}  # 临时输出到构建目录
    )
    get_target_property(output_name ${target_name} OUTPUT_NAME)
    if(NOT output_name)
        set(output_name ${target_name})
    endif()
    set(elf_file ${CMAKE_CURRENT_BINARY_DIR}/${output_name}.elf)
    set(bin_file ${CMAKE_CURRENT_BINARY_DIR}/${output_name}.bin)
    set(hex_file ${CMAKE_CURRENT_BINARY_DIR}/${output_name}.hex)
    set(final_elf ${output_dir}/${output_name}.elf)
    set(final_bin ${output_dir}/${output_name}.bin)
    set(final_hex ${output_dir}/${output_name}.hex)
    # 生成二进制和十六进制文件的命令
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${elf_file} ${bin_file}
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${elf_file} ${hex_file}
        COMMAND ${CMAKE_COMMAND} -E copy ${elf_file} ${final_elf}
        COMMAND ${CMAKE_COMMAND} -E copy ${bin_file} ${final_bin}
        COMMAND ${CMAKE_COMMAND} -E copy ${hex_file} ${final_hex}
    )
    # 输出信息
    if(ENABLE_USER_BIN_OUTPUT)
        message("[${CMAKE_PROJECT_NAME}][info] ${target_name} output confing:")
        message("${log_head_padding}  - temp file : ${CMAKE_CURRENT_BINARY_DIR}/")
        message("${log_head_padding}  - final file: ${output_dir}")
        message("${log_head_padding}  - build file: ${output_name}.elf, ${output_name}.bin, ${output_name}.hex")
    endif()
endfunction()
# 调试语言检测
function(debug_language_detection target_name)
    message("")
    message("[${CMAKE_PROJECT_NAME}][debug] ============ ${target_name} language check ==========")
    get_target_property(source_files_ ${target_name} SOURCES)
    foreach(source_file IN LISTS source_files_)
        get_source_file_property(file_language ${source_file} LANGUAGE)
        message("[${CMAKE_PROJECT_NAME}][debug] file: ${source_file} -> language: ${file_language}")
        # 测试LGE
        if(file_language STREQUAL "C")
            message("[${CMAKE_PROJECT_NAME}][debug]   C LGE: $<$<COMPILE_LANGUAGE:C>:C_LANG>")
        elseif(file_language STREQUAL "CXX")
            message("[${CMAKE_PROJECT_NAME}][debug]   C++ LGE: $<$<COMPILE_LANGUAGE:CXX>:CXX_LANG>")
        elseif(file_language STREQUAL "ASM")
            message("[${CMAKE_PROJECT_NAME}][debug]   ASM LGE: $<$<COMPILE_LANGUAGE:ASM>:ASM_LANG>")
        endif()
    endforeach()
    message("[${CMAKE_PROJECT_NAME}][debug] ============ ${target_name} language check ==========")
    message("")
endfunction()
# 禁用标准库的CMSIS警告
function(disable_cmsis_warnings)
    set(CMSIS_DISABLE_WARNINGS_C
        -Wno-missing-declarations
        -Wno-strict-prototypes
        -Wno-old-style-definition
        -Wno-missing-prototypes
    )
    set(CMSIS_DISABLE_WARNINGS_CXX)
    # 为 OS_INTERNAL_SRC 目标禁用警告（仅对C语言）
    if(TARGET OS_INTERNAL_SRC)
        target_compile_options(OS_INTERNAL_SRC PRIVATE
            $<$<COMPILE_LANGUAGE:C>:${CMSIS_DISABLE_WARNINGS_C}>
            $<$<COMPILE_LANGUAGE:CXX>:${CMSIS_DISABLE_WARNINGS_CXX}>
        )
        message("[${CMAKE_PROJECT_NAME}][note] CMSIS warnings disabled for OS_INTERNAL_SRC (C only)")
    endif()
    # 为 OS_INTERNAL_INC 目标禁用警告（仅对C语言）
    if(TARGET OS_INTERNAL_INC)
        target_compile_options(OS_INTERNAL_INC INTERFACE
            $<$<COMPILE_LANGUAGE:C>:${CMSIS_DISABLE_WARNINGS_C}>
            $<$<COMPILE_LANGUAGE:CXX>:${CMSIS_DISABLE_WARNINGS_CXX}>
        )
        message("[${CMAKE_PROJECT_NAME}][note] CMSIS warnings disabled for OS_INTERNAL_INC (C only)")
    endif()
endfunction()
# 提供接口方便的链接OS内部库
function(target_use_os_internal target_name)
    # 检查目标是否存在
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "[${CMAKE_PROJECT_NAME}][error] Target ${target_name} does not exist")
    endif()
    # 检查OS内部库是否存在
    if(NOT TARGET OS_INTERNAL_SRC)
        message(FATAL_ERROR "[${CMAKE_PROJECT_NAME}][error] OS_INTERNAL_SRC target not found")
    endif()
    # 链接必要的库
    target_link_libraries(${target_name} 
        PRIVATE 
        OS_INTERNAL_SRC
    )
    message("[${CMAKE_PROJECT_NAME}][Tcfg] ${target_name} successfully configured with OS internal libraries:")
    message("${log_head_padding}  - Automatically linked: OS_INTERNAL_SRC")
    message("${log_head_padding}  - Automatically configured: compiler options, linker options")
    message("${log_head_padding}  - CMSIS warnings automatically disabled")
endfunction()
# 创建用户接口库并链接到用户目标
function(target_add_user_includes target_name)
    # 解析用户提供的头文件路径
    set(user_include_paths ${ARGN})
    # 如果没有提供路径，使用默认的USER_INC
    set(all_include_paths ${USER_INC})
    if(user_include_paths)
        list(APPEND all_include_paths ${user_include_paths})
    endif()
    # 检查目标是否存在
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "[${CMAKE_PROJECT_NAME}][error] Target ${target_name} does not exist")
    endif()
    # 创建唯一的接口库名称
    set(interface_lib_name "${target_name}_USER_INC_INTERFACE")
    # 创建接口库
    add_library(${interface_lib_name} INTERFACE)
    # 添加用户头文件路径到接口库
    target_include_directories(${interface_lib_name} INTERFACE ${all_include_paths})
    # 将接口库链接到OS内部库
    target_link_libraries(${interface_lib_name} INTERFACE OS_INTERNAL_INC)
    # 将用户目标链接到接口库
    target_link_libraries(${target_name} PRIVATE ${interface_lib_name})
    message("[${CMAKE_PROJECT_NAME}][Tcfg] Created interface library: ${interface_lib_name}")
    message("[${CMAKE_PROJECT_NAME}][Tcfg] ${target_name} linked with user includes and OS internal libraries")
    # 输出包含路径信息
    if(ENABLE_USER_INC_OUTPUT)
        if(all_include_paths)
            message("[${CMAKE_PROJECT_NAME}][info] User include paths:")
            foreach(inc_path IN LISTS all_include_paths)
                message("${log_head_padding}   - ${inc_path}")
            endforeach()
        endif()
    endif()
endfunction()
# 封装target_add_user_includes target_use_os_internal到一起
function(configure_user_target_include target_name)
    set(other_paths ${ARGN})
    # 自动配置目标
    configure_target(${target_name})
    target_use_os_internal(${target_name})
    target_add_user_includes(${target_name} ${other_paths})
    message("[${CMAKE_PROJECT_NAME}][Tcfg] Success: ${target_name} fully configured with OS environment")
endfunction()
# 生成 clang 配置文件的函数
function(generate_clang_config)
    if(ENABLE_AUTO_CFG_CLANG)
        set(OPTIONS)
        set(ONE_VALUE_ARGS TOOLCHAIN_PATH)
        set(MULTI_VALUE_ARGS)
        cmake_parse_arguments(CLANG_CONFIG 
            "${OPTIONS}" 
            "${ONE_VALUE_ARGS}" 
            "${MULTI_VALUE_ARGS}" 
            ${ARGN}
        )
        set(CLANG_CONFIG_TARGET_MCU "${OS_MCU}")
        # 查找 Python 解释器
        find_package(Python3 COMPONENTS Interpreter QUIET)
        if(NOT Python3_FOUND)
            message("[${CMAKE_PROJECT_NAME}][error] can not find Python3 - skip clang configure")
            message(WARNING "skip clangd configure.")
            return()
        endif()
        # 检查脚本是否存在
        if(NOT EXISTS "${CLANG_CFG_PY_SCRIPT}")
            message("[${CMAKE_PROJECT_NAME}][error] can not find clang config script: ${CLANG_CFG_PY_SCRIPT}")
            message(FATAL_ERROR "can not find clang config script: ${CLANG_CFG_PY_SCRIPT}")
        endif()
        if(NOT EXISTS "${TIDY_TEMPLATE}")
            message("[${CMAKE_PROJECT_NAME}][error] can not find clang config template: ${TIDY_TEMPLATE}")
            message(FATAL_ERROR "can not find clang config template ${TIDY_TEMPLATE}")
        endif()
        if(NOT EXISTS "${CLANGD_TEMPLATE}")
            message("[${CMAKE_PROJECT_NAME}][error] can not find clang config template: ${CLANGD_TEMPLATE}")
            message(FATAL_ERROR "can not find clang config template ${CLANGD_TEMPLATE}")
        endif()
        message("[${CMAKE_PROJECT_NAME}][info] auto generate clang config files...")
        message("[${CMAKE_PROJECT_NAME}][info]   - project toolchain: ${CLANG_CONFIG_TOOLCHAIN_PATH}")
        message("[${CMAKE_PROJECT_NAME}][info]   - target MCU: ${CLANG_CONFIG_TARGET_MCU}")
        # 执行 Python 脚本
        execute_process(
            COMMAND 
                ${Python3_EXECUTABLE} 
                "${CLANG_CFG_PY_SCRIPT}"
                "${CLANG_CONFIG_TOOLCHAIN_PATH}"
                "${CLANG_CONFIG_TARGET_MCU}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE result_code
            OUTPUT_VARIABLE output_text
            ERROR_VARIABLE error_text
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE
        )
        # 处理结果
        if(result_code EQUAL 0)
            message("[${CMAKE_PROJECT_NAME}][info] generate clang config files success")
            if(output_text)
                message(VERBOSE "输出: ${output_text}")
            endif()
        else()
            message("[${CMAKE_PROJECT_NAME}][error] ❌ generate clang config files failed")
            if(error_text)
                message(FATAL_ERROR "${error_text}")
            endif()
            if(output_text)
                message(FATAL_ERROR "${output_text}")
            endif()
        endif()
    endif()
endfunction()