/**
 * @file kernel.hpp
 * @author StratOS Team
 * @brief 系统内核统一入口，聚合所有策略和启动流程
 * @version 1.0.0
 * @date 2026-06-07
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件提供系统内核的顶层模板类 `Kernel`，它聚合了所有策略（内存、调度、
 * 任务管理、硬件抽象等），并提供统一的初始化、启动和任务创建接口。
 *
 * 使用流程：
 * 1. 定义所有策略类，并选择内存布局类型。
 * 2. 实例化 `Kernel` 类型，例如 `using MyKernel = Kernel<...>;`
 * 3. 在 `main()` 中调用 `MyKernel::init()`
 * 4. 调用 `MyKernel::create_task()` 创建任务
 * 5. 调用 `MyKernel::start()` 启动调度器
 *
 * @note 任务对象必须重载 `operator()`，且生命周期必须长于任务本身。
 */

#pragma once

#ifndef STRATOS_KERNEL_KERNEL_HPP
#define STRATOS_KERNEL_KERNEL_HPP

#include "os_config/include/memory_layout.hpp"
#include "os_config/include/memory_layout_type.hpp"
#include "os_hal/include/interrupt.hpp"
#include "os_kernel/include/core/memory/memory.hpp"
#include "os_kernel/include/core/task/scheduler.hpp"
#include "os_kernel/include/core/task/task.hpp"
#include "os_kernel/include/core/types.hpp"
#include <cstddef>
#include <cstdint>

namespace strat_os::kernel
{

/**
 * @brief 系统内核类（静态策略聚合）
 * @tparam AtomicPolicy             原子操作策略
 * @tparam ContextSwitchPolicy      上下文切换策略
 * @tparam DebugPolicy              调试策略
 * @tparam InterruptControllerPolicy中断控制器策略
 * @tparam MpuPolicy                MPU 策略
 * @tparam PlatformContextPolicy    平台上下文策略（用于 TCB）
 * @tparam SystemPolicy             系统控制策略（如休眠、复位）
 * @tparam SystemTickPolicy         系统节拍定时器策略
 * @tparam KernelConfigPolicy       内核类型配置（优先级、ID 类型等）
 * @tparam UserTcbDataPolicy        用户 TCB 扩展数据策略
 * @tparam SchedulerPolicy          调度器策略
 * @tparam TaskPolicy               任务管理器策略
 * @tparam LayoutType               内存布局类型（纯静态/混合/动态）
 * @tparam MaxTasks                 最大任务数（默认 32）
 */
template <typename AtomicPolicy,
          typename ContextSwitchPolicy,
          typename DebugPolicy,
          typename InterruptControllerPolicy,
          typename MpuPolicy,
          typename PlatformContextPolicy,
          typename SystemPolicy,
          typename SystemTickPolicy,
          typename KernelConfigPolicy,
          typename UserTcbDataPolicy,
          typename SchedulerPolicy,
          typename TaskPolicy,
          strat_os::config::MemoryLayoutType LayoutType,
          std::uint32_t MaxTasks = 32>
struct Kernel {
    // ------------------------- 类型别名 -------------------------
    using types_policy                       = KernelConfigPolicy;
    using scheduler_policy                   = SchedulerPolicy;
    using task_policy                        = TaskPolicy;

    using types                              = KernelTypes<types_policy>;
    using tcb                                = Tcb<KernelConfigPolicy, PlatformContextPolicy, UserTcbDataPolicy>;
    using kernel_pool                        = typename Memory<LayoutType>::kernel_pool;
    using kernel_stack_pool                  = typename Memory<LayoutType>::kernel_stack;
    using user_pool                          = typename Memory<LayoutType>::user_pool;
    using user_stack_pool                    = typename Memory<LayoutType>::user_stack;
    using scheduler                          = Scheduler<scheduler_policy>;
    using task                               = Task<task_policy>;

    using priority_type                      = typename types::priority_type;
    using tick_type                          = typename types::tick_type;
    using task_id_type                       = typename types::task_id_type;
    using task_state_type                    = typename types::task_state_type;

    using interrupt_ctrl                     = typename strat_os::hal::InterruptController<InterruptControllerPolicy>;

    static constexpr std::uint32_t max_tasks = MaxTasks;

    // ------------------------- 编译期检查 -------------------------
    // 确保实际分配的内存池大小不小于布局配置中的区域大小
    static_assert(kernel_pool::Policy::size >= strat_os::config::MemoryLayoutConfig::KERNEL_POOL_SIZE,
                  "KernelPool size must be at least KERNEL_POOL_SIZE");
    static_assert(kernel_stack_pool::Policy::size >= strat_os::config::MemoryLayoutConfig::KERNEL_STACK_SIZE,
                  "KernelStackPool size must be at least KERNEL_STACK_SIZE");
    static_assert(user_pool::Policy::size >= strat_os::config::MemoryLayoutConfig::USER_POOL_SIZE,
                  "UserPool size must be at least USER_POOL_SIZE");
    static_assert(user_stack_pool::Policy::size >= strat_os::config::MemoryLayoutConfig::USER_STACK_SIZE,
                  "UserStackPool size must be at least USER_STACK_SIZE");

    // ------------------------- 公共接口 -------------------------
    /**
     * @brief 初始化内核（内存、任务管理器、调度器）
     * @note 必须在创建任何任务之前调用。
     */
    static void init() noexcept {
        interrupt_ctrl::set_priority(interrupt_ctrl::IRQn_Type::PendSV_IRQn, 15);
        task::init();      // 初始化任务管理器（创建空闲任务）
        scheduler::init(); // 初始化调度器
    }

    /**
     * @brief 启动调度器，开始多任务调度
     * @note 此函数不会返回，调用后 CPU 控制权交给调度器。
     */
    static void start() noexcept {
        scheduler::start();
    }

    /**
     * @brief 创建一个新任务
     * @tparam TaskObj 任务对象类型（必须重载 `operator()`）
     * @param task_obj  任务对象引用（其 `operator()` 为任务入口）
     * @param priority  任务优先级
     * @param stack_size 任务栈大小（字节）
     * @return 新任务的 TCB 指针，失败返回 nullptr
     * @note 任务对象必须在整个任务生命周期内有效（通常为静态或全局对象）。
     */
    template <typename TaskObj>
    static tcb* create_task(TaskObj& task_obj, priority_type priority, std::size_t stack_size) noexcept {
        return task::create_task(&task_entry_thunk<TaskObj>, &task_obj, priority, stack_size);
    }

    /**
     * @brief 删除任务（当前未实现）
     * @param task_id 任务 ID
     */
    static void delete_task(task_id_type task_id) noexcept {
        (void)task_id;
        // TODO: 实现任务删除逻辑
    }

  private:
    /**
     * @brief 类型擦除辅助函数，将 `void*` 转换为具体任务对象并调用其 `operator()`
     * @tparam TaskObj 任务对象类型
     * @param ptr 指向任务对象的指针
     */
    template <typename TaskObj>
    static void task_entry_thunk(void* ptr) noexcept {
        TaskObj* obj = static_cast<TaskObj*>(ptr);
        (*obj)(); // 调用任务对象的函数调用运算符
    }
};

} // namespace strat_os::kernel

#endif // STRATOS_KERNEL_KERNEL_HPP