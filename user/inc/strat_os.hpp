/**
 * @file strat_os.hpp
 * @author your name (you@domain.com)
 * @brief 根据用户配置导出内核实例，用户不应该修改此文件。
 * @version 0.1
 * @date 2026-06-07
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifndef STRAT_OS_HPP
#define STRAT_OS_HPP

#include "os_kernel/include/kernel.hpp"
#include "user/inc/os_config.hpp"

namespace strat_os
{
using os_kernel = strat_os::kernel::Kernel<user_config::OsConfig::HalConfig::OsAtomicPolicy,
                                           user_config::OsConfig::HalConfig::OsContextSwitchPolicy,
                                           user_config::OsConfig::HalConfig::OsDebugPolicy,
                                           user_config::OsConfig::HalConfig::OsInterruptCtrlPolicy,
                                           user_config::OsConfig::HalConfig::OsMPUPolicy,
                                           user_config::OsConfig::HalConfig::OsPlatformContextPolicy,
                                           user_config::OsConfig::HalConfig::OsSystemControlPolicy,
                                           user_config::OsConfig::HalConfig::OsSystemTickPolicy,
                                           user_config::OsConfig::KernelConfigPolicy,
                                           user_config::OsConfig::UserTcbDataPolicy,
                                           user_config::OsConfig::SchedulerPolicy,
                                           user_config::OsConfig::TaskPolicy,
                                           user_config::OsConfig::memory_layout>;
}

#endif