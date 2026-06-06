/**
 * @file round_robin.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-06-06
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifndef STRATOS_POLICY_KERNEL_ROUND_ROBIN_HPP
#define STRATOS_POLICY_KERNEL_ROUND_ROBIN_HPP

#include "os_kernel/include/core/tcb.hpp"
#include "os_kernel/include/policy/task/task_lists.hpp"
#include <cstdint>

namespace strat_os::kernel::policy::builtin
{
template <typename KernelConfigPolicy, typename PlatformContextPolicy, typename UserTcbDataPolicy>
struct RoundRobinPolicy {
    /// 任务列表数据结构（包含就绪队列和栈分配器）
    using task_lists = strat_os::kernel::policy::builtin::details::
        TaskLists<KernelConfigPolicy, PlatformContextPolicy, UserTcbDataPolicy>;

    /// 内核类型策略别名
    using kernel_types_policy = KernelConfigPolicy;
    /// 平台上下文策略别名
    using platform_context_policy = PlatformContextPolicy;
    /// 用户 TCB 策略别名
    using user_tcb_policy = UserTcbDataPolicy;

    /// TCB 类型别名
    using tcb_type = strat_os::kernel::Tcb<kernel_types_policy, platform_context_policy, user_tcb_policy>;
    /// 调度器状态类型别名
    using scheduler_state_type = std::uint8_t;

    /// 内核数据类型
    using kernel_types = KernelTypes<kernel_types_policy>;
    /// 优先级类型别名（来自内核类型策略）
    using priority_type = typename kernel_types::priority_type;
    /// 时钟滴答类型别名（来自内核类型策略）
    using tick_type = typename kernel_types::tick_type;

    /**
     * @brief 初始化调度器内部数据结构
     */
    inline static void init() noexcept {
        return;
    }

    /**
     * @brief 启动调度器，开始任务调度
     */
    inline static void start() noexcept {}

    /**
     * @brief 主动让出 CPU，触发重新调度
     */
    inline static void yield() noexcept {}

    /**
     * @brief 执行调度算法，选择下一个要运行的任务
     */
    inline static tcb_type* schedule() noexcept {}

    /**
     * @brief 将任务添加到就绪队列
     * @param task 对 TCB 的常量引用（临时对象）
     * @return 指向就绪队列中持久化 TCB 对象的指针，失败返回 nullptr
     */
    inline static tcb_type* add_task(const tcb_type& task) noexcept {
        if (task_lists::ready_list.full()) return nullptr;
        task_lists::ready_list.push_back(task);
        return &task_lists::ready_list.back();
    }

    /**
     * @brief 从就绪队列中移除任务
     * @param task 指向 TCB 的指针
     */
    inline static void remove_task(tcb_type* task) noexcept {
        task_lists::ready_list.erase(task_lists::ready_list.find(task));
    }
};
} // namespace strat_os::kernel::policy::builtin

#endif // STRATOS_POLICY_KERNEL_ROUND_ROBIN_HPP