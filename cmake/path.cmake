#
# ******************************************************************************
# * @file    path.cmake
# * @author  Yurilt
# * @version V1.0.0
# * @date    03-November-2025
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
include(${CMAKE_SOURCE_DIR}/cmake/private.cmake)
# 自己环境的工具链路径
set(ARM_TOOLCHAIN_PATH "${__ARM_TOOLCHAIN_PATH__}")
# clangd配置脚本路径，请勿修改
set(CLANG_CFG_PY_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/cfg_clangd.py")
# 编译数据库过滤脚本路径，请勿修改
set(CLANG_FILTER_PY_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/filter_compile_commands.py")
# 过滤后的编译数据库路径，应和clangd模板文件一致
set(CLANG_FILTER_JSON_PATH "${CMAKE_SOURCE_DIR}/build/filtered")

# 模板文件
set(TIDY_TEMPLATE "${CMAKE_SOURCE_DIR}/.clang-tidy.template.yml")
set(CLANGD_TEMPLATE "${CMAKE_SOURCE_DIR}/.clangd.template.yml")

# 设置 MUSSTL 安装路径（相对于项目根目录）
set(MUSSTL_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries/MUSSTL/install")
