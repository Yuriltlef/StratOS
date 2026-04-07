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
#include "os_hal/include/debug.hpp"
#include "os_hal/include/interrupt.hpp"
#include "os_hal/include/mpu.hpp"
#include "os_hal/include/platform_context.hpp"
#include "os_hal/include/system_control.hpp"
#include "os_hal/include/system_tick.hpp"

#include "platform/cortex_m3/stm32f1/atomic.hpp"
#include "platform/cortex_m3/stm32f1/context_switch.hpp"
#include "platform/cortex_m3/stm32f1/debug.hpp"
#include "platform/cortex_m3/stm32f1/interrupt.hpp"
#include "platform/cortex_m3/stm32f1/mpu.hpp"
#include "platform/cortex_m3/stm32f1/platform_context.hpp"
#include "platform/cortex_m3/stm32f1/system_control.hpp"
#include "platform/cortex_m3/stm32f1/system_tick.hpp"

#include "os_kernel/config/kernel_config.hpp"
#include "os_kernel/include/core/tcb.hpp"
#include "os_kernel/include/core/types.hpp"


#include <cstdint>

namespace os_builtins                     = strat_os::hal::policy::builtin;
namespace os_kernel_hal                   = strat_os::hal;

using MyPlatformContextPolicy             = os_builtins::CortexM3Stm32F1PlatformContextPolicy;
using MyPlatformContext                   = strat_os::hal::PlatformContext<MyPlatformContextPolicy>;

using MyUserTcbDataPolicy                 = strat_os::kernel::config::DefaultUserTcbDataPolicy;

using MyKernelConfigPolicy                = strat_os::kernel::config::DefaultKernelConfigPolicy;

using MyTcb                               = strat_os::kernel::Tcb<MyKernelConfigPolicy, MyPlatformContextPolicy, MyUserTcbDataPolicy>;

using MyKernelConfig                      = strat_os::kernel::config::DefaultKernelConfig;
using MyTaskState                         = MyKernelConfig::task_state;

using MyCortexM3InterruptControllerPolicy = os_builtins::CortexM3Stm32F1InterruptControllerPolicy;
using MyInterruptController               = os_kernel_hal::InterruptController<MyCortexM3InterruptControllerPolicy>;

using MyCortexM3AtomicPolicy              = os_builtins::CortexM3Stm32F1AtomicPolicy;
using MyAtomic                            = os_kernel_hal::Atomic<MyCortexM3AtomicPolicy>;

using MyCortexM3ContextSwitchPolicy       = os_builtins::CortexM3Stm32F1ContextSwitchPolicy;
using MyContextSwitch                     = os_kernel_hal::ContextSwitch<MyCortexM3ContextSwitchPolicy>;

using MyCortexM3MPUPolicy                 = os_builtins::CortexM3Stm32F1MPUPolicy;
using MyMPU                               = os_kernel_hal::Mpu<MyCortexM3MPUPolicy>;

using MyCortexM3SystemControlPolicy       = os_builtins::CortexM3Stm32F1SystemControlPolicy;
using MySystemControl                     = os_kernel_hal::SystemControl<MyCortexM3SystemControlPolicy>;

using MyCortexM3SystickPolicy             = os_builtins::CortexM3Stm32F1SystemTickPolicy;
using MySystemTick                        = os_kernel_hal::SystemTick<MyCortexM3SystickPolicy>;
using MySystemTickSource                  = MySystemTick::clock_source_type;

using MyCortexM3DebugPolicy               = os_builtins::CortexM3Stm32F1DebugPolicy;
using MyDebug                             = os_kernel_hal::Debug<MyCortexM3DebugPolicy>;

int main() {
    MyDebug::enable_cycle_counter();
    volatile uint32_t i{0};
    while (true) {
        auto __ = (true && false);
        MyInterruptController::global_disable();
        auto i_ = MyTaskState::Terminated;
        auto _  = MyAtomic::add(&i, 1);
        MyContextSwitch::switch_to_privileged();
        auto _p = MyContextSwitch::get_msp();
        MyContextSwitch::switch_to_unprivileged();

        using xyz = MyMPU::region_index_type;

        MySystemControl::set_sleep_on_exit(true);
        static_assert(os_kernel_hal::traits::is_enhanced_fault_controller_v<MySystemControl>,
                      "Not an enhanced fault controller policy");

        MySystemTick::init(0xffffff, MySystemTickSource::AHBClock);
        MySystemTick::enable_irq();
        MySystemTick::enable();

        MyTcb myTcb(nullptr, 0x11, 10);
        myTcb.sp = static_cast<MyTcb::sp_type>(0x20000000);
        
        myTcb.state = MyTcb::task_state_type::Blocked;

        MyDebug::bkpt();

        MyInterruptController::global_enable();
        auto _g = MyInterruptController::get_current_irq();
    }
}
