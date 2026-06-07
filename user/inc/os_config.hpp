/**
 * @file os_config.hpp
 * @author your name (you@domain.com)
 * @brief 此文件为用户工程系统配置文件，负责提供平台抽象层策略和内核策略
 *        用户可自行配置
 * @version 0.1
 * @date 2026-06-07
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifndef USER_OS_CONFIG_HPP
#define USER_OS_CONFIG_HPP

#include "os_hal/config/hal_config.hpp"
#include "os_kernel/config/kernel_config.hpp"
#include "os_kernel/include/policy/task/round_robin.hpp"
#include "os_kernel/include/policy/task/task.hpp"
#include <cstddef> // for std::size_t

namespace user_config
{
struct OsConfig {

    static constexpr std::size_t max_tasks            = 16;

    static constexpr std::size_t time_slice_ticks     = 250;

    static constexpr std::size_t idle_task_stack_size = 128;

    static constexpr auto memory_layout               = strat_os::kernel::config::memory_layout;

    using HalConfig                                   = strat_os::hal::config::OsHalConfig;

    using KernelConfigPolicy                          = strat_os::kernel::config::DefaultKernelConfigPolicy;
    using Kerneltypes                                 = strat_os::kernel::config::DefaultKernelConfig;

    using UserTcbDataPolicy                           = strat_os::kernel::config::DefaultUserTcbDataPolicy;

    using SchedulerPolicy = strat_os::kernel::policy::builtin::RoundRobinPolicy<KernelConfigPolicy,
                                                                                HalConfig::OsPlatformContextPolicy,
                                                                                UserTcbDataPolicy,
                                                                                HalConfig::OsSystemTickPolicy,
                                                                                HalConfig::OsContextSwitchPolicy,
                                                                                memory_layout,
                                                                                max_tasks,
                                                                                idle_task_stack_size,
                                                                                time_slice_ticks>;

    using TaskPolicy      = strat_os::kernel::policy::builtin::TaskPolicy<KernelConfigPolicy,
                                                                          HalConfig::OsPlatformContextPolicy,
                                                                          UserTcbDataPolicy,
                                                                          SchedulerPolicy,
                                                                          HalConfig::OsSystemControlPolicy,
                                                                          HalConfig::OsContextSwitchPolicy,
                                                                          memory_layout,
                                                                          max_tasks,
                                                                          idle_task_stack_size>;
};
} // namespace user_config

#endif // USER_OS_CONFIG_HPP