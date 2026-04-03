/**
 ******************************************************************************
 * @file    main.cpp
 * @author  Yurilt
 * @version V1.0.0
 * @date    03-November-2025
 * @brief   C++主程序入口
 * @note    包含C++程序的main函数和对象初始化
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 Yurilt.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "os_hal/include/atomic.hpp"
#include "os_hal/include/context_switch.hpp"
#include "os_hal/include/interrupt.hpp"
#include "os_hal/include/mpu.hpp"
#include "platform/cortex_m3/stm32f1/interrupt.hpp"
#include "platform/cortex_m3/stm32f1/context_switch.hpp"
#include "platform/cortex_m3/stm32f1/mpu.hpp"
#include "platform/cortex_m3/stm32f1/atomic.hpp"
#include <assert.h>
#include <cstdint>

namespace os_builtins                     = strat_os::hal::policy::builtin;
namespace os_kernel_hal                   = strat_os::hal;

using MyCortexM3InterruptControllerPolicy = os_builtins::CortexM3InterruptControllerPolicy;
using MyInterruptController               = os_kernel_hal::InterruptController<MyCortexM3InterruptControllerPolicy>;

using MyCortexM3AtomicPolicy              = os_builtins::CortexM3AtomicPolicy;
using MyAtomic                            = os_kernel_hal::Atomic<MyCortexM3AtomicPolicy>;

using MyCortexM3ContextSwitchPolicy       = os_builtins::CortexM3ContextSwitchPolicy;
using MyContextSwitch                     = os_kernel_hal::ContextSwitch<MyCortexM3ContextSwitchPolicy>;

using MyCortexM3MPUPolicy                 = os_builtins::CortexM3MPUPolicy;
using MyMPU                               = os_kernel_hal::Mpu<MyCortexM3MPUPolicy>;

int main() {
    volatile uint32_t i{0};
    while (true) {
        MyInterruptController::global_disable();
        auto _ = MyAtomic::add(&i, 1);
        MyContextSwitch::switch_to_privileged();
        auto _p = MyContextSwitch::get_msp();
        MyContextSwitch::switch_to_unprivileged();
        using xyz = MyMPU::region_index_type;
        MyInterruptController::global_enable();
    }
}
