/**
 * @file kernel.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-06-04
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifndef STRATOS_KERNEL_KERNEL_HPP
#define STRATOS_KERNEL_KERNEL_HPP

#include "os_config/include/memory_layout.hpp"
#include "os_config/include/memory_layout_type.hpp"
#include "os_kernel/include/core/memory/memory.hpp"
#include "os_kernel/include/core/task/scheduler.hpp"
#include "os_kernel/include/core/types.hpp"
#include <cstddef>
#include <cstdint>

namespace strat_os::kernel
{
template <typename KernelConfigPolicy,
          typename PlatformContextPolicy,
          typename UserTcbDataPolicy,
          typename SchedulerPolicy,
          typename TaskPolicy,
          config::MemoryLayoutType LayoutType,
          std::uint32_t MaxTasks = 32>
struct Kernel {
    using types                              = KernelTypes<KernelConfigPolicy>;
    using tcb                                = Tcb<KernelConfigPolicy, PlatformContextPolicy, UserTcbDataPolicy>;
    using kernel_pool                        = typename Memory<LayoutType>::kernel_pool;
    using kernel_stack_pool                  = typename Memory<LayoutType>::kernel_stack;
    using user_pool                          = typename Memory<LayoutType>::user_pool;
    using user_stack_pool                    = typename Memory<LayoutType>::user_stack;
    using scheduler                          = Scheduler<SchedulerPolicy>;

    using priority_type                      = typename types::priority_type;
    using tick_type                          = typename types::tick_type;
    using task_id_type                       = typename types::task_id_type;
    using task_state_type                    = typename types::task_state_type;

    constexpr static std::uint32_t max_tasks = MaxTasks;

    static_assert(kernel_pool::Policy::size > strat_os::config::MemoryLayoutConfig::KERNEL_POOL_SIZE,
                  "KernelPool size must be greater than kernel_pool_size in memory layout");
    static_assert(kernel_stack_pool::Policy::size > strat_os::config::MemoryLayoutConfig::KERNEL_STACK_SIZE,
                  "KernelStackPool size must be greater than kernel_stack_pool_size in memory layout");
    static_assert(user_pool::Policy::size > strat_os::config::MemoryLayoutConfig::USER_POOL_SIZE,
                  "UserPool size must be greater than user_pool_size in memory layout");
    static_assert(user_stack_pool::Policy::size > strat_os::config::MemoryLayoutConfig::USER_STACK_SIZE,
                  "UserStackPool size must be greater than user_stack_pool_size in memory layout");

    static void init() noexcept {
        scheduler::init();
    }

    static void start() noexcept {
        scheduler::start();
    }

    template <typename TaskObj>
    static void create_task(TaskObj&& task_obj, priority_type priority, std::size_t stack_size) noexcept {}

    static void delete_task(task_id_type task_id) noexcept {}
};
} // namespace strat_os::kernel
#endif // STRATOS_KERNEL_KERNEL_HPP