/**
 * @file task_lists.hpp
 * @author StratOS Team
 * @brief 时间片轮转调度器的就绪队列数据结构（节点包装模式）
 * @version 1.0.0
 * @date 2026-06-06
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件为时间片轮转（Round‑Robin）调度器定义了就绪队列所需的数据结构。
 * 核心设计采用节点包装（Node Wrapper）模式：
 *   - 定义一个 `tcb_node` 结构体，包含完整的 TCB 对象和侵入式链表指针（prev/next）。
 *   - 就绪队列使用 `mu_sstl::IntrusiveList` 管理所有节点，提供 O(1) 的入队/出队操作。
 *
 * 这种设计带来以下好处：
 * 1. TCB 定义中不需要包含任何链表指针，调度器与任务控制块完全分离。
 * 2. 所有内存分配在编译期完成，链表操作无动态分配。
 * 3. 不依赖特定 TCB 布局，任何 TCB 类型均可通过包装节点用于队列。
 * 4. 每个任务的内存占用为 `sizeof(tcb_node)`，与将指针内嵌到 TCB 的方案完全相同。
 *
 * @note 节点池容量由模板参数 `MaxTasks` 决定，必须大于等于系统实际创建的任务数。
 *       队列控制块（`ready_list_type` 对象）从内核池分配，确保所有内核对象统一管理。
 *
 * @see mu_sstl::IntrusiveList, mu_sstl::StaticAllocPolicy
 */

#pragma once

#ifndef STRATOS_KERNEL_POLICY_TASK_LISTS_HPP
#define STRATOS_KERNEL_POLICY_TASK_LISTS_HPP

#include "mu_sstl/containers/intrusive_list.hpp"
#include "mu_sstl/containers/static_array.hpp"
#include "os_config/include/memory_layout_type.hpp"
#include "os_kernel/include/core/memory/memory.hpp"
#include "os_kernel/include/core/tcb.hpp"
#include "os_kernel/include/core/types.hpp"
#include "user/inc/debug.hpp"
#include <cstddef>

namespace strat_os::kernel::policy::builtin::details
{

/**
 * @brief 时间片轮转调度器就绪队列数据结构模板
 *
 * @tparam KernelConfigPolicy     内核类型配置策略（提供 priority_type、task_id_type 等）
 * @tparam PlatformContextPolicy  平台上下文策略（FPU/MPU 状态）
 * @tparam UserTcbDataPolicy      用户 TCB 扩展数据策略
 * @tparam LayoutType             内存布局类型（默认为纯静态布局）
 * @tparam MaxTasks               最大任务数（默认为 32，必须 ≤ task_id_type 最大值）
 *
 * 该类封装了就绪队列所需的所有类型和静态实例：
 * - `tcb_node`：包装节点，包含 TCB 对象和链表指针。
 * - `ready_list_type`：基于 `IntrusiveList` 的就绪队列类型。
 * - `ready_list`：全局唯一的就绪队列实例（从内核池分配并构造）。
 *
 * @note 队列的构造在静态初始化阶段完成，要求内核池此时已可用。
 *       若内核池需要显式初始化，请确保在访问 `ready_list` 之前调用 `kernel_pool::init()`。
 */
template <typename KernelConfigPolicy,
          typename PlatformContextPolicy,
          typename UserTcbDataPolicy,
          strat_os::config::MemoryLayoutType LayoutType,
          std::uint32_t MaxTasks,
          std::uint32_t IdleTaskStackSize>
struct TaskLists {
    // ========================= 类型别名 =========================
    /// 任务控制块类型（完整 TCB）
    using tcb = Tcb<KernelConfigPolicy, PlatformContextPolicy, UserTcbDataPolicy>;
    /// 内核类型辅助类（提供 task_id_type、priority_type 等）
    using kernel_type = KernelTypes<KernelConfigPolicy>;
    /// 任务 ID 类型（用于链表索引和节点池容量）
    using task_id_type = typename kernel_type::task_id_type;

    // 编译期安全检查
    static_assert(MaxTasks > 0, "MaxTasks must be positive");
    static_assert(MaxTasks <= static_cast<task_id_type>(-1), "MaxTasks exceeds range of task_id_type");

    /// 最大任务数（编译期常量，作为节点池容量）
    constexpr static task_id_type max_tasks = static_cast<task_id_type>(MaxTasks);

    // ========================= 节点定义 =========================
    /**
     * @brief 侵入式链表节点（包装 TCB）
     *
     * 每个节点包含一个完整的 TCB 对象以及前驱/后继索引。
     * 节点池由 `StaticAllocPolicy` 在编译期静态分配，位于 .bss 段。
     *
     * @note 这种包装方式使得 TCB 定义无需包含任何链表指针，实现了调度器与任务控制的解耦。
     *       内存占用 = sizeof(tcb) + 2 * sizeof(task_id_type)，与在 TCB 内部嵌入指针完全等价。
     */
    struct tcb_node {
        using size_type  = task_id_type; ///< 索引类型（用于 prev/next）
        using value_type = tcb*;         ///< 存储的数据类型

        value_type data{}; ///< 实际的任务控制块指针
        size_type prev{};  ///< 前驱节点索引（`npos` 表示无前驱）
        size_type next{};  ///< 后继节点索引

        tcb_node() = default; ///< 默认构造，所有成员零初始化
    };

    /// 节点池分配策略：节点类型为 `tcb_node`，容量为 `max_tasks`
    using ready_list_alloc_policy = mu_sstl::StaticAllocPolicy<tcb_node, task_id_type, max_tasks>;

    // ========================= 内存池别名 =========================
    /// 内核池（用于分配就绪队列控制块对象）
    using kernel_pool = typename Memory<LayoutType>::kernel_pool;
    /// 用户栈池（本调度器未使用，仅保留以便扩展）
    using user_stack = typename Memory<LayoutType>::user_stack;

    /// 就绪队列类型：侵入式链表，节点池容量为 `max_tasks`
    using ready_list_type = mu_sstl::IntrusiveList<ready_list_alloc_policy>;

    /**
     * @brief 全局就绪队列实例（引用）
     *
     * 通过 placement new 在 `ready_list_mem` 指向的内存上构造 `ready_list_type` 对象。
     * 构造函数会初始化空闲链表，将所有节点串联起来。
     *
     * @warning 由于 `static inline` 成员在程序启动时（`main` 之前）初始化，
     *          要求内核池此时已经可用（即已调用 `kernel_pool::init()`）。
     *          如果内核池需要显式初始化，请确保在访问 `ready_list` 之前完成初始化，
     *          或推迟队列的构造到 `SchedulerPolicy::init()` 中。
     */
    static ready_list_type* ready_list;

    static tcb* idle_task;

    constexpr static std::size_t idle_task_stack = IdleTaskStackSize;

    static void init() {
        dxprintf("TaskLists::init(): kernel_pool size = %u, base = 0x%x\n",
                 (unsigned int)kernel_pool::Policy::size,
                 (unsigned int)kernel_pool::Policy::base);

        // 分配就绪队列控制块
        void* mem = kernel_pool::allocate(sizeof(ready_list_type));
        dxprintf("Allocated ready_list control block: 0x%x\n", (unsigned int)mem);
        if (!mem) {
            dprint("ERROR: Failed to allocate ready_list\n");
            while (1) {}
        }
        ready_list = reinterpret_cast<ready_list_type*>(mem);
        new (ready_list) ready_list_type();
        dprint("ready_list constructed.\n");

        // 分配空闲任务 TCB
        mem = kernel_pool::allocate(sizeof(tcb));
        dxprintf("Allocated idle_task TCB: 0x%x\n", (unsigned int)mem);
        if (!mem) {
            dprint("ERROR: Failed to allocate idle_task\n");
            while (1) {}
        }
        idle_task = reinterpret_cast<tcb*>(mem);
        new (idle_task) tcb();
        dprint("idle_task constructed.\n");
    }
};

} // namespace strat_os::kernel::policy::builtin::details

#endif // STRATOS_KERNEL_POLICY_TASK_LISTS_HPP