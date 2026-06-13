/**
 * @file round_robin.hpp
 * @author StratOS Team
 * @brief 时间片轮转调度器策略（支持空闲任务，使用 TCB 指针）
 * @version 2.0.0
 * @date 2026-06-07
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件实现了时间片轮转调度算法，适用于所有任务优先级相同的场景。
 * 调度器维护一个 FIFO 就绪队列，每个任务运行固定的时间片（以系统节拍为单位）。
 * 当任务主动让出 CPU 或时间片用完时，当前任务被移到队尾，调度器选择队首任务运行。
 * 如果就绪队列为空，则运行空闲任务。
 *
 * 主要特性：
 * - 就绪队列存储 TCB 指针，无对象拷贝，符合 TCB 不可拷贝原则。
 * - 支持时间片配置（`enable_timeslice`, `set_timeslice`）。
 * - 支持任务阻塞/唤醒（`block_current`, `unblock_task`）。
 * - 支持主动让出（`yield`）。
 * - 空闲任务由任务管理器静态分配，不消耗时间片。
 *
 * 依赖：
 * - `TaskLists` 提供就绪队列（`IntrusiveList<tcb*>`) 和空闲任务对象。
 * - HAL 层 `ContextSwitch` 提供触发 PendSV、设置/读取 PSP 等操作。
 * - HAL 层 `SystemTick` 提供系统节拍定时器，用于时间片计数。
 *
 * 所有方法均为静态且 noexcept，满足 `Scheduler` 适配器接口要求。
 */

#pragma once

#ifndef STRATOS_POLICY_KERNEL_ROUND_ROBIN_HPP
#define STRATOS_POLICY_KERNEL_ROUND_ROBIN_HPP

#include "os_config/include/memory_layout_type.hpp"
#include "os_hal/include/context_switch.hpp"
#include "os_hal/include/system_tick.hpp"
#include "os_kernel/include/core/tcb.hpp"
#include "os_kernel/include/policy/task/task_lists.hpp"
#include "user/inc/debug.hpp"
#include <cstddef>
#include <cstdint>

namespace strat_os::kernel::policy::builtin
{

/**
 * @brief 时间片轮转调度器策略
 *
 * @tparam KernelConfigPolicy     内核类型配置策略（提供 priority_type、tick_type 等）
 * @tparam PlatformContextPolicy  平台上下文策略（用于 TCB 扩展，此处未使用但保留）
 * @tparam UserTcbDataPolicy      用户 TCB 扩展数据策略
 * @tparam SystemTickPolicy       HAL 系统节拍定时器策略
 * @tparam ContextSwitchPolicy    HAL 上下文切换策略
 */
template <typename KernelConfigPolicy,
          typename PlatformContextPolicy,
          typename UserTcbDataPolicy,
          typename SystemTickPolicy,
          typename ContextSwitchPolicy,
          strat_os::config::MemoryLayoutType LayoutType,
          std::uint32_t MaxTasks,
          std::uint32_t IdleTaskStackSize,
          std::size_t TimeSliceTicks>
struct RoundRobinPolicy {
    // ------------------------- 类型别名 -------------------------
    /// 任务列表数据结构（包含就绪队列、空闲任务等）
    using task_lists              = strat_os::kernel::policy::builtin::details::TaskLists<KernelConfigPolicy,
                                                                                          PlatformContextPolicy,
                                                                                          UserTcbDataPolicy,
                                                                                          LayoutType,
                                                                                          MaxTasks,
                                                                                          IdleTaskStackSize>;

    using kernel_types_policy     = KernelConfigPolicy;
    using platform_context_policy = PlatformContextPolicy;
    using user_tcb_policy         = UserTcbDataPolicy;
    /// TCB 类型（完整任务控制块）
    using tcb_type = strat_os::kernel::Tcb<kernel_types_policy, platform_context_policy, user_tcb_policy>;
    /// 调度器状态类型（占位，无实际统计）
    using scheduler_state_type = std::uint8_t;
    /// 内核类型辅助类
    using kernel_types = KernelTypes<kernel_types_policy>;
    /// 优先级类型
    using priority_type = typename kernel_types::priority_type;
    /// 节拍类型
    using tick_type = typename kernel_types::tick_type;

    /// 系统节拍定时器适配器
    using sys_tick = strat_os::hal::SystemTick<SystemTickPolicy>;
    /// 上下文切换适配器
    using ctx_switch = strat_os::hal::ContextSwitch<ContextSwitchPolicy>;

    static constexpr std::size_t default_slice = TimeSliceTicks;

    // ------------------------- 静态成员 -------------------------
    /// 当前正在运行的任务指针
    static tcb_type* current_task;
    /// 默认时间片长度（节拍数）
    static tick_type time_slice_ticks;
    /// 每个任务剩余的节拍数（按任务 ID 索引，数组从内核池分配）
    static tick_type* time_left;
    /// 最大任务数（编译期常量）
    static constexpr std::size_t max_tasks = task_lists::max_tasks;

  private:
    // ------------------------- 辅助函数 -------------------------
    /**
     * @brief 将当前任务放回就绪队列尾部
     * @note 仅当当前任务有效且不是空闲任务时执行。当前任务正在运行，不应在就绪队列中，
     *       因此直接使用 `push_back` 将其指针加入队尾。
     */
    static void requeue_current() noexcept {
        if (!current_task || current_task == task_lists::idle_task) return;
        if (task_lists::ready_list->full()) return;      // 队列满（不应发生）
        task_lists::ready_list->push_back(current_task); // 存储指针
    }

  public:
    // ------------------------- 必需方法 -------------------------
    /**
     * @brief 初始化调度器内部数据结构
     * @note 从内核池分配 `time_left` 数组并清零，重置当前任务指针。
     */
    static void init() noexcept {
        void* mem = task_lists::kernel_pool::allocate(sizeof(tick_type) * max_tasks);
        time_left = reinterpret_cast<tick_type*>(mem);
        for (std::size_t i = 0; i < max_tasks; ++i)
            time_left[i] = 0;
        current_task = nullptr;
    }

    /**
     * @brief 启动调度器，开始任务调度
     * @details 从就绪队列中取出第一个任务（或空闲任务），设置 PSP，
     *          初始化 SysTick 定时器并触发第一次 PendSV 切换。
     */
    static void start() noexcept {
        // 确保第一个任务已就绪，并设置 PSP
        if (!task_lists::ready_list->empty()) {
            tcb_type* first = task_lists::ready_list->front();
            // task_lists::ready_list->pop_front();
            current_task = first;
            ctx_switch::set_psp(static_cast<typename ctx_switch::word>(current_task->sp) + 32);
        } else {
            // 没有用户任务，使用空闲任务
            current_task = task_lists::idle_task;
            ctx_switch::set_psp(static_cast<typename ctx_switch::word>(current_task->sp) + 32);
        }
        dprint("set psp done.\n");
        // 使能系统节拍定时器（注意：时钟源参数需根据实际策略定义）
        sys_tick::init(0x1193F, sys_tick::clock_source_type::AHBClock);
        sys_tick::enable_irq();
        sys_tick::enable();
        // 触发第一次 PendSV（此时 PSP 已正确设置）
        // __asm volatile("msr control, %0" : : "r"(0x2) : "memory");
        // __asm volatile("isb");
        ctx_switch::trigger_pendsv();
    }

    /**
     * @brief 主动让出 CPU，触发重新调度
     * @details 将当前任务放回就绪队列尾部，然后触发 PendSV 进行任务切换。
     */
    static void yield() noexcept {
        requeue_current();            // 将当前任务放回队尾
        ctx_switch::trigger_pendsv(); // 触发切换
    }

    /**
     * @brief 执行调度算法，选择下一个要运行的任务
     * @return 下一个任务的 TCB 指针（永远不会为 nullptr，因为空闲任务保证可用）
     * @details 如果就绪队列非空，取出队首任务并弹出；否则返回空闲任务。
     */
    [[nodiscard]] static tcb_type* schedule() noexcept {
        if (!task_lists::ready_list->empty()) {
            tcb_type* next = task_lists::ready_list->front();
            task_lists::ready_list->pop_front();
            dxprintf("after pop_front: next->sp=0x%x\n", next->sp);
            current_task = next;
            return current_task;
        } else {
            tcb_type* ret = (current_task ? current_task : task_lists::idle_task);
            dxprintf("schedule: returning %x (current_task=%x, idle=%x)\n",
                     (void*)ret,
                     (void*)current_task,
                     (void*)task_lists::idle_task);
            return ret;
        }
    }

    /**
     * @brief 将任务添加到就绪队列
     * @param task 指向 TCB 的指针（必须由内核池分配，生命周期有效）
     * @return 相同的 TCB 指针，若队列满则返回 nullptr
     * @note 同时初始化该任务的剩余时间片为默认值。
     */
    [[nodiscard]] static tcb_type* add_task(tcb_type* task) noexcept {
        if (!task) return nullptr;
        if (task_lists::ready_list->full()) return nullptr;
        task_lists::ready_list->push_back(task);
        if (task->id < max_tasks) {
            time_left[task->id] = time_slice_ticks;
        }
        return task;
    }

    /**
     * @brief 从就绪队列中移除任务
     * @param task 指向 TCB 的指针
     * @note 时间片轮转调度器通常不需要显式删除任务，此接口留空。
     */
    static void remove_task(tcb_type* task) noexcept {
        (void)task; // 未使用，占位
    }

    /**
     * @brief 阻塞当前正在运行的任务
     * @details 将当前任务状态改为 `Blocked`，并触发重新调度。
     *          当前任务正在运行，不在就绪队列中，因此无需从队列中移除。
     */
    static void block_current() noexcept {
        if (current_task && current_task != &task_lists::idle_task) {
            current_task->state = kernel_types::task_state_type::Blocked;
        }
        ctx_switch::trigger_pendsv();
    }

    /**
     * @brief 唤醒（解除阻塞）指定的任务
     * @param task 指向 TCB 的指针
     * @details 将任务状态改为 `Ready`，并加入就绪队列尾部。
     */
    static void unblock_task(tcb_type* task) noexcept {
        if (task->state == kernel_types::task_state_type::Blocked) {
            task->state = kernel_types::task_state_type::Ready;
            if (!task_lists::ready_list->full()) {
                task_lists::ready_list->push_back(task);
            }
        }
    }

    /**
     * @brief 获取当前正在运行的任务的 TCB 指针
     * @return 当前任务的 TCB 指针
     */
    [[nodiscard]] static tcb_type* get_current() noexcept {
        return current_task;
    }

    /**
     * @brief 设置当前正在运行的任务
     * @param task 指向 TCB 的指针
     * @note 通常仅由调度器内部调用。
     */
    static void set_current(tcb_type* task) noexcept {
        current_task = task;
    }

    /**
     * @brief 时钟滴答处理，由系统节拍中断调用
     * @details 递减当前任务的剩余时间片，若归零则重置时间片，
     *          将当前任务移到就绪队列尾部，并触发重新调度。
     *          空闲任务不消耗时间片。
     */
    static void tick() noexcept {
        if (!current_task) return;
        if (current_task == task_lists::idle_task) return; // 空闲任务不消耗时间片

        if (time_left[current_task->id] > 0) {
            --time_left[current_task->id];
        }
        if (time_left[current_task->id] == 0) {
            time_left[current_task->id] = time_slice_ticks; // 重置时间片
            requeue_current();                              // 将当前任务移到队尾
            ctx_switch::trigger_pendsv();                   // 触发重新调度
        }
    }

    // ----- 可选方法（时间片管理）-----
    /**
     * @brief 使能时间片轮转调度并设置默认时间片长度
     * @param ticks 每个任务可运行的时间片长度（以系统滴答为单位）
     */
    static void enable_timeslice(tick_type ticks) noexcept {
        time_slice_ticks = ticks;
        sys_tick::init(ticks, sys_tick::clock_source_type::AHBClock);
    }

    /**
     * @brief 设置指定任务的时间片长度
     * @param tcb   指向 TCB 的指针
     * @param ticks 新的时间片长度
     */
    static void set_timeslice(tcb_type* tcb, tick_type ticks) noexcept {
        if (tcb && tcb->id < max_tasks) {
            time_left[tcb->id] = ticks;
        }
    }

    // ----- 可选方法（调度器锁）-----
    static void lock_scheduler() noexcept {}
    static void unlock_scheduler() noexcept {}
    static bool is_scheduler_locked() noexcept {
        return false;
    }

    // ----- 可选方法（空闲任务）-----
    static void set_idle_task(tcb_type* tcb) noexcept {
        (void)tcb;
    }
    [[nodiscard]] static tcb_type* get_idle_task() noexcept {
        return task_lists::idle_task;
    }

    // ----- 可选方法（统计信息）-----
    [[nodiscard]] static scheduler_state_type get_statistics() noexcept {
        return 0;
    }
};

} // namespace strat_os::kernel::policy::builtin

#endif // STRATOS_POLICY_KERNEL_ROUND_ROBIN_HPP