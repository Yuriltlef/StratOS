#
# arm-none-eabi-gcc 工具链路径，示例："C:/ARM_TOOLCHAIN/10 2021.10/"
#
include(${CMAKE_SOURCE_DIR}/cmake/private.cmake)

# 自己环境的工具链路径
set(ARM_TOOLCHAIN_PATH "${__ARM_TOOLCHAIN_PATH__}")

# clangd配置脚本路径，请勿修改
set(CLANG_CFG_PY_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/cfg_clangd.py")

# 模板文件
set(TIDY_TEMPLATE "${CMAKE_SOURCE_DIR}/.clang-tidy.template")
set(CLANGD_TEMPLATE "${CMAKE_SOURCE_DIR}/.clangd.template")
