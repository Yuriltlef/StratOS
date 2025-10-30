#
# ******************************************************************************
# * @file    os_confing.cmake
# * @author  Yurilt
# * @version V1.0.0
# * @date    31-October-2025
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
set(OS_VERSION
    0.0.1
)
# ====================== 项目名 ======================
set(OS_PROJECT_NAME
    StratOS
)
# ====================== 项目描述 ======================
set(OS_DESCRIPTION
    "a RTOS for ARM Cortex-M3 using Template-based Policy Design and GNU Toolchain."
)
# ======================输出ELF文件名======================
set(OS_ELF_TARGET 
    ${OS_PROJECT_NAME}
)
# ====================== C标准 ======================
set(OS_C_STANDARD 
    11
)
# ====================== CPP标准 ======================
set(OS_CPP_STANDARD 
    17
)
# ====================== core ======================
set(OS_MCU
    cortex-m3
)
# ====================== 预定义宏 ======================
set(OS_STM_MODEL
    STM32F10X_MD
)
set(OS_STM_LIB_TYPE_FLAG
    USE_STDPERIPH_DRIVER
)
set(OS_CONFING
    OS_STM_RAM_SIZE=0x5000
)
# ====================== arm cortex编译选项 ======================
set(OS_CPU_FLAGS
    -mthumb -mcpu=${OS_MCU}
)
# ==================== C CPP通用的编译选项 ======================
set(OS_MUTUAL_FLAGS
    ${OS_CPU_FLAGS}
    -fdata-sections
    -ffunction-sections
    -fstack-usage
    -specs=nano.specs
    -specs=nosys.specs
)
# ==================== C 编译选项 ======================
set(OS_C_FLAGS
    -fno-strict-aliasing
)
set(OS_C_WARNNING_FLAGS
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wold-style-definition
    -Wmissing-declarations
)
set(OS_C_DEBUG_FLAGS
    -O0
    -g3
    -gdwarf-2 
    -fno-omit-frame-pointer
    -fno-inline
)
set(OS_C_RELEASE_FLAGS
    -O3
    -flto
    -fomit-frame-pointer
)
set(OS_C_MINSIZEREL_FLAGS
    -Os
    -flto
    -fomit-frame-pointer
    -DNDEBUG
)
set(OS_C_RELWITHDEBINFO_FLAGS
    -O2
    -g3
    -gdwarf-2
    -flto
    -DNDEBUG
)
# ==================== CPP 编译选项 ======================
set(OS_CPP_FLAGS
    -fno-exceptions
    -fno-unwind-tables
    -fno-rtti
    -fno-use-cxa-atexit
    -fno-threadsafe-statics
    -fno-sized-deallocation
    -fno-weak
)
set(OS_CPP_WARNNING_FLAGS
    -Wnon-virtual-dtor
    -Woverloaded-virtual
    -Wctor-dtor-privacy
    -Wno-register
)
set(OS_CPP_DEBUG_FLAGS
    -O0
    -g3
    -gdwarf-2 
    -fno-omit-frame-pointer
    -fno-inline
    -fno-elide-constructors
)
set(OS_CPP_RELEASE_FLAGS
    -O3
    -flto
    -fomit-frame-pointer
)
set(OS_CPP_MINSIZEREL_FLAGS
    -Os
    -flto
    -fomit-frame-pointer
    -DNDEBUG      
)
set(OS_CPP_RELWITHDEBINFO_FLAGS
    -O2
    -g3
    -gdwarf-2
    -flto
    -DNDEBUG
)
# ==================== asm 编译选项 ======================
set(OS_ASM_FLAGS
    ${OS_CPU_FLAGS}
    -x assembler-with-cpp
    -Wa,--warn
)
# ==================== ld 选项 ======================
set(OS_LD_BASE_FLAGS
    -T ${OS_LD_SCRIPT}
    ${OS_CPU_FLAGS}
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,--cref
    -Wl,--print-memory-usage
)
set(OS_CPP_LD_FLAGS
    -Wl,--no-wchar-size-warning
    -u _printf_float
    -u _scanf_float
)
set(OS_LD_DEBUG_FLAGS
    -Wl,--warn-common
    -Wl,--warn-once
)
set(OS_LD_RELEASE_FLAGS
    -Wl,--strip-debug
)
set(OS_LD_MINSIZEREL_FLAGS
    -Wl,--strip-debug
)
set(OS_LD_RELWITHDEBINFO_FLAGS
    -Wl,--warn-common
)
# ==================== CPP 特定链接脚本 ======================
set(OS_CPP_LD_FLAGS
    -lstdc++ 
    -lsupc++ 
    -lm 
    -lc 
    -lgcc
)
# ==================== 构建输出设置 ======================
# 输出完整编译命令
option(ENABLE_FULL_COMPILE_OPTIONS BOOL)
set(ENABLE_FULL_COMPILE_OPTIONS ON)
# 输出完整链接命令
option(ENABLE_FULL_LD_OPTIONS BOOL)
set(ENABLE_FULL_LD_OPTIONS ON)
# 测试语言生成表达式LGE
option(ENABLE_LGE_CHECK BOOL)
set(ENABLE_LGE_CHECK ON)
# 输出用户自定义INC目录
option(ENABLE_USER_INC_OUTPUT BOOL)
set(ENABLE_USER_INC_OUTPUT ON)
# 输出用户自定义bin output目录
option(ENABLE_USER_BIN_OUTPUT BOOL)
set(ENABLE_USER_BIN_OUTPUT ON)
# 自动配置clangd
option(ENABLE_AUTO_CFG_CLANG BOOL)
set(ENABLE_AUTO_CFG_CLANG ON)
