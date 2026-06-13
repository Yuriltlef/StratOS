/**
 * @file task.hpp
 * @author StratOS Team
 * @brief 内置任务管理器策略类（用于时间片轮转调度器）
 * @version 1.0.0
 * @date 2026-06-06
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件实现了任务管理器的默认策略类 `TaskPolicy`，用于时间片轮转（Round‑Robin）调度器。
 * 它利用 `TaskLists` 中定义的静态就绪队列和线性栈分配器，提供任务创建、查询、优先级修改等基本功能。
 *
 * 该策略类符合 StratOS 的静态策略模式，可作为模板参数传递给 `strat_os::kernel::Task` 适配器。
 *
 * @note 当前版本为演示实现，仅支持：
 *       - 创建任务（自动分配栈并加入就绪队列）
 *       - 查询任务 ID 和状态
 *       - 修改任务优先级
 *       - 挂起、恢复、删除任务暂未实现（仅占位）
 *
 * @see strat_os::kernel::policy::builtin::details::TaskLists
 * @see strat_os::kernel::Task
 */

#pragma once

#ifndef STRATOS_POLICY_KERNEL_TASK_HPP
#define STRATOS_POLICY_KERNEL_TASK_HPP

#include "os_config/include/memory_layout_type.hpp"
#include "os_hal/include/context_switch.hpp"
#include "os_hal/include/system_control.hpp"
#include "os_kernel/include/core/task/scheduler.hpp"
#include "os_kernel/include/core/tcb.hpp"
#include "os_kernel/include/policy/task/task_lists.hpp"
#include <cstddef>
#include <cstdint>

namespace strat_os::kernel::policy::builtin
{

/**
 * @brief 内置任务管理器策略类
 *
 * @tparam KernelConfigPolicy     内核类型配置策略（提供 priority_type、task_id_type、task_state_type 等）
 * @tparam PlatformContextPolicy  平台上下文策略（用于 TCB 扩展，此处未使用但保留）
 * @tparam UserTcbDataPolicy      用户 TCB 扩展数据策略（此处未使用但保留）
 *
 * 该策略实现了以下核心操作：
 * - 创建任务：分配栈空间，构造 TCB，加入就绪队列。
 * - 获取任务 ID 和状态。
 * - 修改任务优先级。
 *
 * 所有方法均为静态且 noexcept，保证零开销。
 *
 * @note 当前版本依赖 `TaskLists` 中定义的 `ready_list`（`IntrusiveList<tcb>`）
 *       和 `user_stack`（线性栈分配器）。
 */
template <typename KernelConfigPolicy,
          typename PlatformContextPolicy,
          typename UserTcbDataPolicy,
          typename SchedulerPolicy,
          typename SystemPolicy,
          typename ContextSwitchPolicy,
          strat_os::config::MemoryLayoutType LayoutType,
          std::uint32_t MaxTasks,
          std::uint32_t IdleTaskStackSize>
struct TaskPolicy {
    // ==================== 类型别名 ====================
    /// 任务列表数据结构（包含就绪队列和栈分配器）
    using task_lists = strat_os::kernel::policy::builtin::details::TaskLists<KernelConfigPolicy,
                                                                             PlatformContextPolicy,
                                                                             UserTcbDataPolicy,
                                                                             LayoutType,
                                                                             MaxTasks,
                                                                             IdleTaskStackSize>;

    /// 调度器接口
    using scheduler = strat_os::kernel::Scheduler<SchedulerPolicy>;

    /// 内核类型策略（直接透传）
    using kernel_types_policy = KernelConfigPolicy;
    /// 平台上下文策略（透传）
    using platform_context_policy = PlatformContextPolicy;
    /// 用户 TCB 扩展策略（透传）
    using user_tcb_policy = UserTcbDataPolicy;

    /// TCB 具体类型（由三个策略实例化）
    using tcb_type = strat_os::kernel::Tcb<kernel_types_policy, platform_context_policy, user_tcb_policy>;

    /// 内核类型辅助类（提供类型别名）
    using kernel_types = KernelTypes<kernel_types_policy>;

    /// 优先级类型（来自内核配置）
    using priority_type = typename kernel_types::priority_type;
    /// 系统节拍类型（保留用于未来扩展）
    using tick_type = typename kernel_types::tick_type;
    /// 任务 ID 类型
    using task_id_type = typename kernel_types::task_id_type;
    /// 任务状态枚举类型
    using task_state_type = typename kernel_types::task_state_type;

    using sys_ctrl        = strat_os::hal::SystemControl<SystemPolicy>;
    using ctx_switch      = strat_os::hal::ContextSwitch<ContextSwitchPolicy>;

    /// id计数器
    static task_id_type tid;

    static void idle(void*) {
        while (true) {
            sys_ctrl::sleep();
        }
    }

    // ==================== 必需方法 ====================

    /**
     * @brief 初始化任务管理器，创建 idle 任务并设置调度器的空闲任务
     */
    inline static void init() noexcept {
        task_lists::init();
        void* stack_mem = task_lists::user_stack::allocate(task_lists::idle_task_stack);

        if (!stack_mem) {
            return; /// 错误处理
        }

        task_lists::idle_task->task     = nullptr;

        task_lists::idle_task->entry    = &idle;

        task_lists::idle_task->id       = 33;

        task_lists::idle_task->priority = 0;

        task_lists::idle_task->state    = tcb_type::task_state_type::Ready;

        std::uint32_t new_sp            = ctx_switch::init_stack(task_lists::idle_task->entry,
                                                                 task_lists::idle_task->task,
                                                                 reinterpret_cast<std::uintptr_t>(stack_mem));

        task_lists::idle_task->sp       = new_sp;
    }

    /**
     * @brief 创建一个新任务并将其加入就绪队列
     *
     * @param entry      任务入口函数（参数为 `void*` 的任务对象指针）
     * @param task_obj   任务对象指针（在 C++ 任务类中通常为 `this`，C 风格任务可传 `nullptr`）
     * @param prio       任务优先级（时间片轮转调度中通常忽略，但保留用于未来优先级扩展）
     * @param stack_size 任务栈大小（字节）
     * @return 新任务的 TCB 指针，若栈分配失败或队列已满则返回 `nullptr`
     *
     * @note 实现细节：
     *       1. 通过 `task_lists::user_stack::allocate(stack_size)` 分配栈空间，
     *          该函数返回的地址已经是栈顶（高地址），可直接赋值给 `tcb.sp`。
     *       2. 临时构造一个 `tcb_type` 对象，使用当前队列长度作为任务 ID（演示用途）。
     *       3. 调用 `ready_list.push_back` 将 TCB 复制到侵入式链表的节点池中。
     *       4. 返回链表中尾部元素的地址（即持久化 TCB 对象的指针）。
     *
     * @warning 如果就绪队列已满（`ready_list.full()`），`push_back` 会触发错误处理器（默认死循环）。
     *          调用前应检查 `ready_list.available() > 0`，但本示例未做检查以简化代码。
     */
    [[nodiscard]] static tcb_type* create_task(void (*entry)(void*),
                                               void* task_obj,
                                               priority_type prio,
                                               std::size_t stack_size) noexcept {
        // 分配栈空间（返回栈顶地址）
        void* stack_mem = task_lists::user_stack::allocate(stack_size);
        if (!stack_mem) {
            return nullptr;
        }

        std::uintptr_t stack_top = reinterpret_cast<std::uintptr_t>(stack_mem);

        // 初始化栈帧
        stack_top = ctx_switch::init_stack(entry, task_obj, stack_top);

        // 分配tcb空间
        void* tcb_mem = task_lists::kernel_pool::allocate(sizeof(tcb_type));
        if (!tcb_mem) {
            return nullptr;
        }
        // 构造 TCB 对象
        tcb_type* new_tcb = new (tcb_mem) tcb_type(entry, task_obj, prio, tid);
        new_tcb->state    = tcb_type::task_state_type::Ready;
        new_tcb->sp       = stack_top;
        tid++;
        // 将 TCB 加入就绪队列，返回持久 TCB 指针
        return scheduler::add_task(new_tcb);
    }

    /**
     * @brief 删除任务（当前版本未实现）
     * @param tcb 要删除的任务的 TCB 指针
     * @note 仅作为占位，实际删除需要从就绪队列中移除节点，并释放栈空间。
     *       演示版本不做实现，调用此函数无效果。
     */
    inline static void destroy_task(tcb_type* tcb) noexcept {
        (void)tcb; // 避免未使用参数警告
        // TODO: 实现任务删除（从就绪队列移除、释放栈、回收节点）
    }

    /**
     * @brief 挂起任务（当前版本未实现）
     * @param tcb 要挂起的任务
     * @note 占位函数，实际应修改任务状态并从就绪队列中移除。
     */
    inline static void suspend_task(tcb_type* tcb) noexcept {
        (void)tcb;
        // TODO: 实现挂起逻辑
    }

    /**
     * @brief 恢复挂起的任务（当前版本未实现）
     * @param tcb 要恢复的任务
     * @note 占位函数，实际应将任务重新加入就绪队列。
     */
    inline static void resume_task(tcb_type* tcb) noexcept {
        (void)tcb;
        // TODO: 实现恢复逻辑
    }

    /**
     * @brief 动态改变任务优先级
     * @param tcb      目标任务
     * @param new_prio 新优先级
     * @note 时间片轮转调度器中优先级通常相同，但该函数仍会修改 TCB 中的优先级字段，
     *       以备未来扩展优先级调度。
     */
    inline static void set_priority(tcb_type* tcb, priority_type new_prio) noexcept {
        tcb->priority = new_prio;
    }

    // ==================== 可选扩展方法 ====================

    /**
     * @brief 获取任务唯一标识符
     * @param tcb 目标任务
     * @return 任务 ID（存储在 TCB 中）
     */
    [[nodiscard]] inline static task_id_type get_task_id(tcb_type* tcb) noexcept {
        return tcb->id;
    }

    /**
     * @brief 获取任务当前状态
     * @param tcb 目标任务
     * @return 任务状态枚举（就绪、运行、阻塞等）
     */
    [[nodiscard]] inline static task_state_type get_task_state(tcb_type* tcb) noexcept {
        return tcb->state;
    }
};

} // namespace strat_os::kernel::policy::builtin

#endif // STRATOS_POLICY_KERNEL_TASK_HPP