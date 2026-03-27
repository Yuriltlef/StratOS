# ====================================================================
# logUtils.cmake - CMake日志实用工具
#
# 提供四种日志函数：
#   log_warning(message)    - 输出警告信息
#   log_error(message)      - 输出错误信息（可选择是否终止）
#   log_info(message)       - 输出一般信息
#   log_custom(head, message) - 输出自定义头信息
#
# 输出格式：
#   [project_name][time][<head>] <message>
#
# 示例：
#   [MyProject][2024-01-01 12:00:00][WARNING] This is a warning message.
#   [MyProject][2024-01-01 12:00:00][ERROR] This is an error message.
#   [MyProject][2024-01-01 12:00:00][INFO] This is an info message.
#   [MyProject][2024-01-01 12:00:00][CUSTOM] This is a custom message.
#
# 版本: 1.0.0
# 作者: AI Assistant
# ====================================================================

# ====================================================================
# 配置选项
# ====================================================================

# 默认项目名称（可在调用前设置）
if(NOT DEFINED LOG_PROJECT_NAME)
    set(LOG_PROJECT_NAME "Project" CACHE STRING "Default project name for log messages")
endif()

# 日志级别颜色支持
option(LOG_ENABLE_COLORS "Enable colored output" ON)

# 时间格式
if(NOT DEFINED LOG_TIME_FORMAT)
    set(LOG_TIME_FORMAT "%Y-%m-%d %H:%M:%S" CACHE STRING "Time format for log messages")
endif()

# ====================================================================
# 颜色定义
# ====================================================================

if()
    # 生成ESC字符
    string(ASCII 27 ESC)
    set(COLOR_RESET   "${ESC}[0m")
    set(COLOR_RED     "${ESC}[31m")
    set(COLOR_GREEN   "${ESC}[32m")
    set(COLOR_YELLOW  "${ESC}[33m")
    set(COLOR_BLUE    "${ESC}[34m")
    set(COLOR_MAGENTA "${ESC}[35m")
    set(COLOR_CYAN    "${ESC}[36m")
    set(COLOR_WHITE   "${ESC}[37m")
    set(COLOR_BOLD    "${ESC}[1m")
    
    # 各日志级别颜色
    set(LOG_COLOR_ERROR   "${COLOR_RED}${COLOR_BOLD}")
    set(LOG_COLOR_WARNING "${COLOR_YELLOW}${COLOR_BOLD}")
    set(LOG_COLOR_INFO    "${COLOR_GREEN}")
    set(LOG_COLOR_CUSTOM  "${COLOR_CYAN}")
    set(LOG_COLOR_TIME    "${COLOR_WHITE}")
    set(LOG_COLOR_PROJECT "${COLOR_MAGENTA}")
    set(LOG_COLOR_HEAD    "${COLOR_BOLD}")
else()
    # 无颜色版本
    set(COLOR_RESET   "")
    set(LOG_COLOR_ERROR   "")
    set(LOG_COLOR_WARNING "")
    set(LOG_COLOR_INFO    "")
    set(LOG_COLOR_CUSTOM  "")
    set(LOG_COLOR_TIME    "")
    set(LOG_COLOR_PROJECT "")
    set(LOG_COLOR_HEAD    "")
endif()

# ====================================================================
# 内部辅助函数
# ====================================================================

# 获取当前时间字符串
function(_log_get_timestamp out_var)
    string(TIMESTAMP current_time "${LOG_TIME_FORMAT}" UTC)
    set(${out_var} "${current_time}" PARENT_SCOPE)
endfunction()

# 生成日志前缀
function(_log_generate_prefix head out_prefix)
    # 获取时间戳
    _log_get_timestamp(timestamp)
    
    # 生成带颜色的前缀
    set(prefix 
        "${LOG_COLOR_PROJECT}[${LOG_PROJECT_NAME}]${COLOR_RESET}"
        "${LOG_COLOR_TIME}[${timestamp}]${COLOR_RESET}"
        "${LOG_COLOR_HEAD}[${head}]${COLOR_RESET}"
    )
    
    # 合并前缀
    string(REPLACE ";" "" prefix "${prefix}")
    
    set(${out_prefix} "${prefix}" PARENT_SCOPE)
endfunction()

# ====================================================================
# 公共日志函数
# ====================================================================

# log_warning - 输出警告信息
# 用法: log_warning("message")
function(log_warning message)
    _log_generate_prefix("WARNING" prefix)
    message("${prefix} ${LOG_COLOR_WARNING}${message}${COLOR_RESET}")
endfunction()

# log_error - 输出错误信息
# 用法: log_error("message" [FATAL])
# 参数:
#   message - 错误消息
#   FATAL  - 可选，如果提供则终止构建
function(log_error message)
    _log_generate_prefix("ERROR" prefix)
    
    # 检查是否有FATAL参数
    set(is_fatal FALSE)
    foreach(arg ${ARGN})
        if(arg STREQUAL "FATAL")
            set(is_fatal TRUE)
        endif()
    endforeach()
    
    if(is_fatal)
        message(FATAL_ERROR "${prefix} ${LOG_COLOR_ERROR}${message}${COLOR_RESET}")
    else()
        message(SEND_ERROR "${prefix} ${LOG_COLOR_ERROR}${message}${COLOR_RESET}")
    endif()
endfunction()

# log_info - 输出一般信息
# 用法: log_info("message")
function(log_info message)
    _log_generate_prefix("INFO" prefix)
    message(STATUS "${prefix} ${LOG_COLOR_INFO}${message}${COLOR_RESET}")
endfunction()

# log_custom - 输出自定义头信息
# 用法: log_custom("head" "message")
function(log_custom head message)
    _log_generate_prefix("${head}" prefix)
    message("${prefix} ${LOG_COLOR_CUSTOM}${message}${COLOR_RESET}")
endfunction()

# ====================================================================
# 高级日志函数
# ====================================================================

# log_section - 输出章节标题
# 用法: log_section("title")
function(log_section title)
    message("")
    message("${COLOR_BOLD}${COLOR_CYAN}=====================================================================${COLOR_RESET}")
    message("${COLOR_BOLD}${COLOR_CYAN}  ${title}${COLOR_RESET}")
    message("${COLOR_BOLD}${COLOR_CYAN}=====================================================================${COLOR_RESET}")
    message("")
endfunction()

# log_success - 输出成功信息
# 用法: log_success("message")
function(log_success message)
    _log_generate_prefix("SUCCESS" prefix)
    message("${prefix} ${COLOR_GREEN}${COLOR_BOLD}✓ ${message}${COLOR_RESET}")
endfunction()

# log_failure - 输出失败信息
# 用法: log_failure("message")
function(log_failure message)
    _log_generate_prefix("FAILURE" prefix)
    message("${prefix} ${COLOR_RED}${COLOR_BOLD}✗ ${message}${COLOR_RESET}")
endfunction()

# log_debug - 调试信息（仅当DEBUG模式启用时输出）
# 用法: log_debug("message")
function(log_debug message)
    if(LOG_DEBUG_ENABLED OR DEBUG)
        _log_generate_prefix("DEBUG" prefix)
        message("${prefix} ${COLOR_MAGENTA}${message}${COLOR_RESET}")
    endif()
endfunction()

# ====================================================================
# 配置函数
# ====================================================================

# log_set_project_name - 设置项目名称
# 用法: log_set_project_name("MyProject")
function(log_set_project_name project_name)
    set(LOG_PROJECT_NAME "${project_name}" CACHE STRING "Project name for log messages" FORCE)
    log_info("Project name set to: ${LOG_PROJECT_NAME}")
endfunction()

# log_set_time_format - 设置时间格式
# 用法: log_set_time_format("%Y/%m/%d %H:%M:%S")
function(log_set_time_format time_format)
    set(LOG_TIME_FORMAT "${time_format}" CACHE STRING "Time format for log messages" FORCE)
    log_info("Time format set to: ${LOG_TIME_FORMAT}")
endfunction()

# log_enable_debug - 启用调试日志
# 用法: log_enable_debug()
function(log_enable_debug)
    set(LOG_DEBUG_ENABLED TRUE CACHE BOOL "Enable debug logs" FORCE)
    log_info("Debug logging enabled")
endfunction()

# log_disable_debug - 禁用调试日志
# 用法: log_disable_debug()
function(log_disable_debug)
    set(LOG_DEBUG_ENABLED FALSE CACHE BOOL "Enable debug logs" FORCE)
    log_info("Debug logging disabled")
endfunction()

# ====================================================================
# 测试函数（可选）
# ====================================================================

# log_run_tests - 运行所有日志函数测试
# 用法: log_run_tests()
function(log_run_tests)
    log_section("Log Utils Test Suite")
    
    log_info("Starting log utilities tests...")
    
    # 测试各种日志级别
    log_info("This is an information message.")
    log_warning("This is a warning message.")
    log_error("This is a non-fatal error message.")
    
    # 测试自定义头
    log_custom("BUILD" "Building target: my_target")
    log_custom("CONFIG" "Configuration: Release")
    log_custom("INSTALL" "Installing to: /usr/local")
    
    # 测试高级函数
    log_success("Operation completed successfully")
    log_failure("Operation failed")
    
    # 测试调试信息（如果启用）
    log_debug("Debug information: variable = ${CMAKE_VERSION}")
    
    log_info("Log utilities tests completed.")
    
    # 注意：我们不测试fatal error，因为它会终止构建
    # log_error("This would be a fatal error" FATAL)
endfunction()

# ====================================================================
# 使用示例（注释）
# ====================================================================

# 示例用法:
#
# # 在CMakeLists.txt中
# include(logUtils.cmake)
# 
# # 设置项目名称
# log_set_project_name("MyEmbeddedProject")
# 
# # 配置阶段
# log_section("Configuration")
# log_info("Configuring for platform: ${CMAKE_SYSTEM_NAME}")
# log_info("C++ standard: ${CMAKE_CXX_STANDARD}")
# 
# # 检查依赖
# find_package(Threads REQUIRED)
# if(Threads_FOUND)
#     log_success("Threads library found")
# else()
#     log_error("Threads library not found" FATAL)
# endif()
# 
# # 编译阶段
# log_section("Building Targets")
# add_executable(my_app main.cpp)
# log_info("Target added: my_app")
# 
# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#     log_warning("Debug build may have performance overhead")
# endif()
# 
# # 安装阶段
# log_section("Installation")
# install(TARGETS my_app DESTINATION bin)
# log_success("Installation configured")
# 
# # 自定义构建步骤
# add_custom_command(
#     OUTPUT generated_file.cpp
#     COMMAND ${PYTHON_EXECUTABLE} generate.py
#     COMMENT "Generating source file"
# )
# log_custom("GENERATE", "Added custom command for code generation")


