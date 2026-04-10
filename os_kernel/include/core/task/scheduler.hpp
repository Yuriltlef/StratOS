/**
 * @file scheduler.hpp
 * @author StratOS Team
 * @brief 调度器策略接口与适配器
 * @version 1.0.0
 * @date 2026-04-09
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了内核调度器的抽象接口，采用静态策略模式。
 * 它将调度相关的核心操作（任务添加/移除、上下文切换、阻塞/唤醒、时间片管理等）
 * 封装为策略类，并通过类型萃取在编译期检测策略类的能力，最终通过适配器模板 `Scheduler`
 * 提供统一的静态接口供 RTOS 内核使用。
 *
 * 调度器功能分为必需和可选两类：
 * - 必需功能：初始化、启动、主动让出、调度、任务增删、阻塞/唤醒、获取/设置当前任务、
 *   时钟滴答处理 —— 所有调度器策略必须实现。
 * - 可选功能：时间片管理、调度器锁、空闲任务、统计信息 —— 由策略选择性实现，
 *   适配器通过 SFINAE 自动暴露相应接口。
 *
 * 该设计保证了零开销抽象，所有方法均为内联且 noexcept，适合嵌入式实时系统。
 *
 * @note
 * - 调度器策略类的所有方法必须为 `static` 且 `noexcept`。
 * - 策略类必须提供嵌套类型 `tcb_type`、`scheduler_state_type`、`kernel_types_policy`、
 *   `platform_context_policy`、`user_tcb_policy`。
 * - `tcb_type` 必须是 `Tcb<kernel_types_policy, platform_context_policy, user_tcb_policy>` 的实例。
 * - 调度器适配器通过 `std::enable_if_t` 和 `traits` 在编译期检查策略完整性，
 *   不满足要求的策略将触发 `static_assert` 并给出明确的错误信息。
 *
 * @warning
 * - 调度器方法通常在中断上下文或临界区中被调用，实现必须保证可重入性和最小延迟。
 * - `schedule()` 方法绝不能阻塞或调用可能阻塞的操作（如信号量获取），否则可能导致死锁。
 * - `tick()` 方法由系统定时器中断调用，实现必须非常轻量，避免长时间循环或动态内存分配。
 * - 时间片管理相关方法（`enable_timeslice`、`set_timeslice`）仅在策略支持时可用，
 *   使用前应检查策略特性，否则会导致编译错误。
 * - 调度器锁（`lock_scheduler`/`unlock_scheduler`）必须支持嵌套锁定（允许同一上下文多次锁定），
 *   且解锁次数需与锁定次数匹配，以防止调度器意外解锁。
 *
 * @attention
 * - 策略类的 `add_task` 和 `remove_task` 必须正确维护就绪队列的内部状态，
 *   特别是当任务被删除时，需确保没有悬空指针残留。
 * - `block_current` 和 `unblock_task` 必须与同步原语（信号量、事件标志等）协同工作，
 *   避免在阻塞期间丢失唤醒信号。
 * - `get_current` 和 `set_current` 必须保持一致性，`set_current` 通常仅由调度器内部调用，
 *   外部不应直接修改当前任务指针。
 * - 若策略实现统计信息（`get_statistics`），应注意统计数据的读取应当是原子的或使用无锁结构。
 */

#pragma once

#ifndef STRATOS_KERNEL_SCHEDULER_HPP
#define STRATOS_KERNEL_SCHEDULER_HPP

#include "os_kernel/include/core/common_traits.hpp" // for has_init, has_init_v
#include "os_kernel/include/core/tcb.hpp"           // for Tcb
#include <type_traits>                              // for false_type, true_type, void_t, is_same
#include <utility>                                  // for declval

namespace strat_os::kernel::traits
{

// -----------------------------------------------------------------------------
// 调度器策略核心类型检测
// -----------------------------------------------------------------------------

/**
 * @brief 检测类型 T 是否包含嵌套类型 tcb_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_tcb_type : std::false_type {};
template <typename T>
struct has_tcb_type<T, std::void_t<typename T::tcb_type>> : std::true_type {};
template <typename T>
static constexpr bool has_tcb_type_v = has_tcb_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 scheduler_state_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_scheduler_state_type : std::false_type {};
template <typename T>
struct has_scheduler_state_type<T, std::void_t<typename T::scheduler_state_type>> : std::true_type {};
template <typename T>
static constexpr bool has_scheduler_state_type_v = has_scheduler_state_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 user_tcb_policy
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_user_tcb_policy_type : std::false_type {};
template <typename T>
struct has_user_tcb_policy_type<T, std::void_t<typename T::user_tcb_policy>> : std::true_type {};
template <typename T>
static constexpr bool has_user_tcb_policy_type_v = has_user_tcb_policy_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 platform_context_policy
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_platform_context_policy_type : std::false_type {};
template <typename T>
struct has_platform_context_policy_type<T, std::void_t<typename T::platform_context_policy>> : std::true_type {};
template <typename T>
static constexpr bool has_platform_context_policy_type_v = has_platform_context_policy_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 kernel_types_policy
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_kernel_types_policy_type : std::false_type {};
template <typename T>
struct has_kernel_types_policy_type<T, std::void_t<typename T::kernel_types_policy>> : std::true_type {};
template <typename T>
static constexpr bool has_kernel_types_policy_type_v = has_kernel_types_policy_type<T>::value;

// -----------------------------------------------------------------------------
// 必需方法检测（调度器核心操作）
// -----------------------------------------------------------------------------

/**
 * @brief 检测静态方法 start()
 */
template <typename T, typename = void>
struct has_start_method : std::false_type {};
template <typename T>
struct has_start_method<T, std::void_t<decltype(T::start())>> : std::true_type {};
template <typename T>
static constexpr bool has_start_method_v = has_start_method<T>::value;

/**
 * @brief 检测静态方法 yield()
 */
template <typename T, typename = void>
struct has_yield_method : std::false_type {};
template <typename T>
struct has_yield_method<T, std::void_t<decltype(T::yield())>> : std::true_type {};
template <typename T>
static constexpr bool has_yield_method_v = has_yield_method<T>::value;

/**
 * @brief 检测静态方法 schedule()
 */
template <typename T, typename = void>
struct has_schedule_method : std::false_type {};
template <typename T>
struct has_schedule_method<T, std::void_t<decltype(T::schedule())>> : std::true_type {};
template <typename T>
static constexpr bool has_schedule_method_v = has_schedule_method<T>::value;

/**
 * @brief 检测静态方法 add_task(tcb_type*)
 */
template <typename T, typename = void>
struct has_add_task_method : std::false_type {};
template <typename T>
struct has_add_task_method<T, std::void_t<decltype(T::add_task(std::declval<typename T::tcb_type*>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_add_task_method_v = has_add_task_method<T>::value;

/**
 * @brief 检测静态方法 remove_task(tcb_type*)
 */
template <typename T, typename = void>
struct has_remove_task_method : std::false_type {};
template <typename T>
struct has_remove_task_method<T, std::void_t<decltype(T::remove_task(std::declval<typename T::tcb_type*>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_remove_task_method_v = has_remove_task_method<T>::value;

/**
 * @brief 检测静态方法 block_current()
 */
template <typename T, typename = void>
struct has_block_current_method : std::false_type {};
template <typename T>
struct has_block_current_method<T, std::void_t<decltype(T::block_current())>> : std::true_type {};
template <typename T>
static constexpr bool has_block_current_method_v = has_block_current_method<T>::value;

/**
 * @brief 检测静态方法 unblock_task(tcb_type*)
 */
template <typename T, typename = void>
struct has_unblock_task_method : std::false_type {};
template <typename T>
struct has_unblock_task_method<T, std::void_t<decltype(T::unblock_task(std::declval<typename T::tcb_type*>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_unblock_task_method_v = has_unblock_task_method<T>::value;

/**
 * @brief 检测静态方法 get_current()
 */
template <typename T, typename = void>
struct has_get_current_method : std::false_type {};
template <typename T>
struct has_get_current_method<T, std::void_t<decltype(T::get_current())>> : std::true_type {};
template <typename T>
static constexpr bool has_get_current_method_v = has_get_current_method<T>::value;

/**
 * @brief 检测静态方法 set_current(tcb_type*)
 */
template <typename T, typename = void>
struct has_set_current_method : std::false_type {};
template <typename T>
struct has_set_current_method<T, std::void_t<decltype(T::set_current(std::declval<typename T::tcb_type*>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_set_current_method_v = has_set_current_method<T>::value;

/**
 * @brief 检测静态方法 tick()
 */
template <typename T, typename = void>
struct has_tick_method : std::false_type {};
template <typename T>
struct has_tick_method<T, std::void_t<decltype(T::tick())>> : std::true_type {};
template <typename T>
static constexpr bool has_tick_method_v = has_tick_method<T>::value;

// -----------------------------------------------------------------------------
// 返回类型正确性检测
// -----------------------------------------------------------------------------

/**
 * @brief 检测 get_current() 的返回类型是否为 tcb_type*
 */
template <typename T, typename = void>
struct is_get_current_return_type : std::false_type {};
template <typename T>
struct is_get_current_return_type<T, std::void_t<decltype(T::get_current())>>
    : std::is_same<decltype(T::get_current()), typename T::tcb_type*> {};
template <typename T>
static constexpr bool is_get_current_return_type_v = is_get_current_return_type<T>::value;

// -----------------------------------------------------------------------------
// TCB 类型有效性检测
// -----------------------------------------------------------------------------

/**
 * @brief 检测策略的 tcb_type 是否与 Tcb<...> 匹配
 * @tparam T 待检测的调度器策略类型
 * @details 要求 T 必须包含嵌套类型 tcb_type, user_tcb_policy, platform_context_policy,
 *          kernel_types_policy，且 tcb_type 是由这些策略实例化的 Tcb。
 */
template <typename T, typename = void, typename = void, typename = void, typename = void>
struct is_valid_tcb_type : std::false_type {};
template <typename T>
struct is_valid_tcb_type<T,
                         std::void_t<typename T::tcb_type>,
                         std::void_t<typename T::user_tcb_policy>,
                         std::void_t<typename T::platform_context_policy>,
                         std::void_t<typename T::kernel_types_policy>>
    : std::is_same<
          typename T::tcb_type,
          Tcb<typename T::kernel_types_policy, typename T::platform_context_policy, typename T::user_tcb_policy>> {};
template <typename T>
static constexpr bool is_valid_tcb_type_v = is_valid_tcb_type<T>::value;

// -----------------------------------------------------------------------------
// 完整策略有效性组合检测
// -----------------------------------------------------------------------------

/**
 * @brief 组合检测，判断类型 T 是否为有效的调度器策略
 * @tparam T 待检测的类型
 *
 * 要求 T 必须提供：
 * - 嵌套类型 tcb_type, scheduler_state_type, kernel_types_policy,
 *   platform_context_policy, user_tcb_policy
 * - 静态方法 init(), start(), yield(), schedule(), add_task(), remove_task(),
 *   block_current(), unblock_task(), get_current(), set_current(), tick()
 * - get_current() 返回类型为 tcb_type*
 * - tcb_type 必须是有效的 Tcb 实例化
 */
template <typename T>
struct is_valid_scheduler_policy : std::conjunction<has_tcb_type<T>,
                                                    has_scheduler_state_type<T>,
                                                    has_kernel_types_policy_type<T>,
                                                    has_platform_context_policy_type<T>,
                                                    has_user_tcb_policy_type<T>,
                                                    has_init_method<T>,
                                                    has_start_method<T>,
                                                    has_yield_method<T>,
                                                    has_schedule_method<T>,
                                                    has_add_task_method<T>,
                                                    has_remove_task_method<T>,
                                                    has_block_current_method<T>,
                                                    has_unblock_task_method<T>,
                                                    has_get_current_method<T>,
                                                    has_set_current_method<T>,
                                                    has_tick_method<T>,
                                                    is_get_current_return_type<T>,
                                                    is_valid_tcb_type<T>> {};
template <typename T>
static constexpr bool is_valid_scheduler_policy_v = is_valid_scheduler_policy<T>::value;

// -----------------------------------------------------------------------------
// 可选功能检测
// -----------------------------------------------------------------------------

/**
 * @brief 检测静态方法 enable_timeslice(tick_type)
 */
template <typename T, typename = void>
struct has_enable_timeslice_method : std::false_type {};
template <typename T>
struct has_enable_timeslice_method<
    T,
    std::void_t<decltype(T::enable_timeslice(std::declval<typename T::kernel_types_policy::tick_type>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_enable_timeslice_method_v = has_enable_timeslice_method<T>::value;

/**
 * @brief 检测静态方法 set_timeslice(tcb_type*, tick_type)
 */
template <typename T, typename = void>
struct has_set_timeslice_method : std::false_type {};
template <typename T>
struct has_set_timeslice_method<
    T,
    std::void_t<decltype(T::set_timeslice(std::declval<typename T::tcb_type*>(),
                                          std::declval<typename T::kernel_types_policy::tick_type>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_set_timeslice_method_v = has_set_timeslice_method<T>::value;

/**
 * @brief 检测静态方法 lock_scheduler()
 */
template <typename T, typename = void>
struct has_lock_scheduler_method : std::false_type {};
template <typename T>
struct has_lock_scheduler_method<T, std::void_t<decltype(T::lock_scheduler())>> : std::true_type {};
template <typename T>
static constexpr bool has_lock_scheduler_method_v = has_lock_scheduler_method<T>::value;

/**
 * @brief 检测静态方法 unlock_scheduler()
 */
template <typename T, typename = void>
struct has_unlock_scheduler_method : std::false_type {};
template <typename T>
struct has_unlock_scheduler_method<T, std::void_t<decltype(T::unlock_scheduler())>> : std::true_type {};
template <typename T>
static constexpr bool has_unlock_scheduler_method_v = has_unlock_scheduler_method<T>::value;

/**
 * @brief 检测静态方法 is_scheduler_locked()
 */
template <typename T, typename = void>
struct has_is_scheduler_locked_method : std::false_type {};
template <typename T>
struct has_is_scheduler_locked_method<T, std::void_t<decltype(T::is_scheduler_locked())>> : std::true_type {};
template <typename T>
static constexpr bool has_is_scheduler_locked_method_v = has_is_scheduler_locked_method<T>::value;

/**
 * @brief 检测 is_scheduler_locked() 的返回类型是否为 bool
 */
template <typename T, typename = void>
struct is_correct_is_scheduler_locked_return_type : std::false_type {};
template <typename T>
struct is_correct_is_scheduler_locked_return_type<T, std::void_t<decltype(T::is_scheduler_locked())>>
    : std::is_same<bool, decltype(T::is_scheduler_locked())> {};
template <typename T>
static constexpr bool is_correct_is_scheduler_locked_return_type_v =
    is_correct_is_scheduler_locked_return_type<T>::value;

/**
 * @brief 检测静态方法 set_idle_task(tcb_type*)
 */
template <typename T, typename = void>
struct has_set_idle_task_method : std::false_type {};
template <typename T>
struct has_set_idle_task_method<T, std::void_t<decltype(T::set_idle_task(std::declval<typename T::tcb_type*>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_set_idle_task_method_v = has_set_idle_task_method<T>::value;

/**
 * @brief 检测静态方法 get_idle_task()
 */
template <typename T, typename = void>
struct has_get_idle_task_method : std::false_type {};
template <typename T>
struct has_get_idle_task_method<T, std::void_t<decltype(T::get_idle_task())>> : std::true_type {};
template <typename T>
static constexpr bool has_get_idle_task_method_v = has_get_idle_task_method<T>::value;

/**
 * @brief 检测 get_idle_task() 的返回类型是否为 tcb_type*
 */
template <typename T, typename = void>
struct is_correct_has_get_idle_task_return_type : std::false_type {};
template <typename T>
struct is_correct_has_get_idle_task_return_type<T, std::void_t<decltype(T::has_get_idle_task())>>
    : std::is_same<typename T::tcb_type*, decltype(T::has_get_idle_task())> {};
template <typename T>
static constexpr bool is_correct_has_get_idle_task_return_type_v = is_correct_has_get_idle_task_return_type<T>::value;

/**
 * @brief 检测静态方法 get_statistics()
 */
template <typename T, typename = void>
struct has_get_statistics_method : std::false_type {};
template <typename T>
struct has_get_statistics_method<T, std::void_t<decltype(T::get_statistics())>> : std::true_type {};
template <typename T>
static constexpr bool has_get_statistics_method_v = has_get_statistics_method<T>::value;

/**
 * @brief 检测 get_statistics() 的返回类型是否为 scheduler_state_type
 */
template <typename T, typename = void>
struct is_correct_get_statistics_return_type : std::false_type {};
template <typename T>
struct is_correct_get_statistics_return_type<T, std::void_t<decltype(T::get_statistics())>>
    : std::is_same<typename T::scheduler_state_type, decltype(T::get_statistics())> {};
template <typename T>
static constexpr bool is_correct_get_statistics_return_type_v = is_correct_get_statistics_return_type<T>::value;

} // namespace strat_os::kernel::traits

namespace strat_os::kernel
{

/**
 * @brief 调度器适配器模板
 * @tparam SchedulerPolicy 具体的策略类，必须满足调度器策略接口
 *
 * 该类将策略类包装为统一的静态接口，并进行编译期验证。
 * 所有方法均为内联且 noexcept，转发到策略类的对应静态方法。
 *
 * 可选功能（如时间片管理、调度器锁、空闲任务、统计信息）通过 SFINAE 条件暴露，
 * 若策略未实现，则相应方法在适配器中不可用，从而避免代码膨胀。
 *
 * 使用示例：
 * @code
 * using MyScheduler = Scheduler<RoundRobinPolicy>;
 * MyScheduler::init();
 * MyScheduler::add_task(task);
 * MyScheduler::start();
 * @endcode
 */
template <typename SchedulerPolicy, typename = std::enable_if_t<traits::is_valid_scheduler_policy_v<SchedulerPolicy>>>
struct Scheduler {
    /// 策略类型别名
    using Policy = SchedulerPolicy;

    /// 内核类型策略别名
    using kernel_types_policy = typename Policy::kernel_types_policy;
    /// 平台上下文策略别名
    using platform_context_policy = typename Policy::platform_context_policy;
    /// 用户 TCB 策略别名
    using user_tcb_policy = typename Policy::user_tcb_policy;
    /// TCB 类型别名
    using tcb_type = typename Policy::tcb_type;
    /// 调度器状态类型别名
    using scheduler_state_type = typename Policy::scheduler_state_type;

    /// 优先级类型别名（来自内核类型策略）
    using priority_type = typename kernel_types_policy::priority_type;
    /// 时钟滴答类型别名（来自内核类型策略）
    using tick_type = typename kernel_types_policy::tick_type;

    // ----- 细粒度静态断言，提供清晰的错误信息 -----
    static_assert(traits::has_tcb_type_v<Policy>, "Policy must provide nested type 'tcb_type'");
    static_assert(traits::has_scheduler_state_type_v<Policy>, "Policy must provide nested type 'scheduler_state_type'");
    static_assert(traits::has_kernel_types_policy_type_v<Policy>,
                  "Policy must provide nested type 'kernel_types_policy'");
    static_assert(traits::has_platform_context_policy_type_v<Policy>,
                  "Policy must provide nested type 'platform_context_policy'");
    static_assert(traits::has_user_tcb_policy_type_v<Policy>, "Policy must provide nested type 'user_tcb_policy'");
    static_assert(traits::has_init_method_v<Policy>, "Policy must provide init()");
    static_assert(traits::has_start_method_v<Policy>, "Policy must provide start()");
    static_assert(traits::has_yield_method_v<Policy>, "Policy must provide yield()");
    static_assert(traits::has_schedule_method_v<Policy>, "Policy must provide schedule()");
    static_assert(traits::has_add_task_method_v<Policy>, "Policy must provide add_task(tcb_type*)");
    static_assert(traits::has_remove_task_method_v<Policy>, "Policy must provide remove_task(tcb_type*)");
    static_assert(traits::has_block_current_method_v<Policy>, "Policy must provide block_current()");
    static_assert(traits::has_unblock_task_method_v<Policy>, "Policy must provide unblock_task(tcb_type*)");
    static_assert(traits::has_get_current_method_v<Policy>, "Policy must provide get_current()");
    static_assert(traits::has_set_current_method_v<Policy>, "Policy must provide set_current(tcb_type*)");
    static_assert(traits::has_tick_method_v<Policy>, "Policy must provide tick()");
    static_assert(traits::is_get_current_return_type_v<Policy>, "Policy's get_current() must return tcb_type*");
    static_assert(traits::is_valid_tcb_type_v<Policy>,
                  "Policy::tcb_type must be Tcb<kernel_types_policy, platform_context_policy, user_tcb_policy>");

    // ----- 必需方法（所有调度器必须实现）-----

    /**
     * @brief 初始化调度器内部数据结构
     */
    inline static void init() noexcept {
        Policy::init();
    }

    /**
     * @brief 启动调度器，开始任务调度
     */
    inline static void start() noexcept {
        Policy::start();
    }

    /**
     * @brief 主动让出 CPU，触发重新调度
     */
    inline static void yield() noexcept {
        Policy::yield();
    }

    /**
     * @brief 执行调度算法，选择下一个要运行的任务
     */
    inline static void schedule() noexcept {
        Policy::schedule();
    }

    /**
     * @brief 将任务添加到就绪队列
     * @param task 指向 TCB 的指针
     */
    inline static void add_task(tcb_type* task) noexcept {
        Policy::add_task(task);
    }

    /**
     * @brief 从就绪队列中移除任务
     * @param task 指向 TCB 的指针
     */
    inline static void remove_task(tcb_type* task) noexcept {
        Policy::remove_task(task);
    }

    /**
     * @brief 阻塞当前正在运行的任务
     */
    inline static void block_current() noexcept {
        Policy::block_current();
    }

    /**
     * @brief 唤醒（解除阻塞）指定的任务
     * @param task 指向 TCB 的指针
     */
    inline static void unblock_task(tcb_type* task) noexcept {
        Policy::unblock_task(task);
    }

    /**
     * @brief 获取当前正在运行的任务的 TCB 指针
     * @return 当前任务的 TCB 指针
     */
    [[nodiscard]] inline static tcb_type* get_current() noexcept {
        return Policy::get_current();
    }

    /**
     * @brief 设置当前正在运行的任务
     * @param task 指向 TCB 的指针
     */
    inline static void set_current(tcb_type* task) noexcept {
        Policy::set_current(task);
    }

    /**
     * @brief 时钟滴答处理，通常由系统定时器中断调用
     */
    inline static void tick() noexcept {
        Policy::tick();
    }

    // ----- 可选方法（时间片管理）-----

    /**
     * @brief 使能时间片轮转调度
     * @param ticks 每个任务可运行的时间片长度（以系统滴答为单位）
     * @note 仅当策略提供 enable_timeslice() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_enable_timeslice_method_v<P>>>
    inline static void enable_timeslice(tick_type ticks) noexcept {
        P::enable_timeslice(ticks);
    }

    /**
     * @brief 设置指定任务的时间片长度
     * @param tcb 指向 TCB 的指针
     * @param ticks 新的时间片长度
     * @note 仅当策略提供 set_timeslice() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_set_timeslice_method_v<P>>>
    inline static void set_timeslice(tcb_type* tcb, tick_type ticks) noexcept {
        P::set_timeslice(tcb, ticks);
    }

    // ----- 可选方法（调度器锁）-----

    /**
     * @brief 锁定调度器，禁止任务切换
     * @note 仅当策略提供 lock_scheduler() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_lock_scheduler_method_v<P>>>
    inline static void lock_scheduler() noexcept {
        P::lock_scheduler();
    }

    /**
     * @brief 解锁调度器，允许任务切换
     * @note 仅当策略提供 unlock_scheduler() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_unlock_scheduler_method_v<P>>>
    inline static void unlock_scheduler() noexcept {
        P::unlock_scheduler();
    }

    /**
     * @brief 查询调度器当前是否被锁定
     * @return true 已锁定，false 未锁定
     * @note 仅当策略提供 is_scheduler_locked() 且返回类型为 bool 时可用
     */
    template <typename P = Policy,
              typename   = std::enable_if_t<traits::has_is_scheduler_locked_method_v<P> &&
                                            traits::is_correct_is_scheduler_locked_return_type_v<P>>>
    inline static bool is_scheduler_locked() noexcept {
        static_assert(traits::is_correct_is_scheduler_locked_return_type_v<P>,
                      "is_scheduler_locked() method must return bool");
        return P::is_scheduler_locked();
    }

    // ----- 可选方法（空闲任务）-----

    /**
     * @brief 设置空闲任务
     * @param tcb 指向空闲任务 TCB 的指针
     * @note 仅当策略提供 set_idle_task() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_set_idle_task_method_v<P>>>
    inline static void set_idle_task(tcb_type* tcb) noexcept {
        P::set_idle_task(tcb);
    }

    /**
     * @brief 获取空闲任务的 TCB 指针
     * @return 空闲任务的 TCB 指针
     * @note 仅当策略提供 get_idle_task() 且返回类型为 tcb_type* 时可用
     */
    template <typename P = Policy,
              typename   = std::enable_if_t<traits::has_get_idle_task_method_v<P> &&
                                            traits::is_correct_has_get_idle_task_return_type_v<P>>>
    inline static tcb_type* get_idle_task() noexcept {
        static_assert(traits::is_correct_has_get_idle_task_return_type_v<P>,
                      "is_scheduler_locked() method must return tcb_type*");
        return P::get_idle_task();
    }

    // ----- 可选方法（统计信息）-----

    /**
     * @brief 获取调度器统计信息
     * @return 调度器状态结构体
     * @note 仅当策略提供 get_statistics() 且返回类型为 scheduler_state_type 时可用
     */
    template <typename P = Policy,
              typename   = std::enable_if_t<traits::has_get_statistics_method_v<P> &&
                                            traits::is_correct_get_statistics_return_type_v<P>>>
    inline static scheduler_state_type get_statistics() noexcept {
        static_assert(traits::is_correct_get_statistics_return_type_v<P>,
                      "is_scheduler_locked() method must return scheduler_state_type");
        return P::get_statistics();
    }
};

} // namespace strat_os::kernel

#endif // STRATOS_KERNEL_SCHEDULER_HPP