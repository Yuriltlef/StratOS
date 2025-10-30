#
# ******************************************************************************
# * @file    path.cmake
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
include(${CMAKE_SOURCE_DIR}/cmake/private.cmake)
# 自己环境的工具链路径
set(ARM_TOOLCHAIN_PATH "${__ARM_TOOLCHAIN_PATH__}")
# clangd配置脚本路径，请勿修改
set(CLANG_CFG_PY_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/cfg_clangd.py")
# 模板文件
set(TIDY_TEMPLATE "${CMAKE_SOURCE_DIR}/.clang-tidy.template")
set(CLANGD_TEMPLATE "${CMAKE_SOURCE_DIR}/.clangd.template")
