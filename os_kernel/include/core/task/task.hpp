/**
 * @file task.hpp
 * @author StratOS Team
 * @brief 任务管理器策略适配器（TaskManager）
 * @version 1.3.0
 * @date 2026-06-06
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了任务管理器的策略适配器模板 `Task`。任务管理器负责任务的
 * 生命周期管理（创建、删除、挂起、恢复、优先级变更），但不包含调度器
 * 的核心调度操作（如阻塞/唤醒、主动让出等），这些由 `Scheduler` 适配器负责。
 *
 * 任务管理器策略类（TaskPolicy）必须提供以下嵌套类型：
 * - kernel_types_policy   : 内核类型配置策略
 * - platform_context_policy: 平台上下文策略
 * - user_tcb_policy       : 用户 TCB 扩展策略
 * - tcb_type              : TCB 类型（必须符合 Tcb 模板实例）
 *
 * 策略类还必须提供以下静态方法：
 * - init()                                  : 初始化任务管理器（创建 idle 任务等）
 * - create_task(entry, task, prio, stack_size) -> tcb_type*
 * - destroy_task(tcb)                       : 删除任务
 * - suspend_task(tcb)                       : 挂起任务
 * - resume_task(tcb)                        : 恢复任务
 * - set_priority(tcb, new_prio)             : 改变任务优先级
 *
 * 可选方法（通过 SFINAE 检测）：
 * - delay(ticks)                            : 当前任务延时
 * - delay_until(absolute_ticks)             : 绝对延时
 * - get_task_id(tcb) -> task_id_type        : 获取任务 ID
 * - get_task_state(tcb) -> task_state_type  : 获取任务状态
 * - task_notify(tcb, value, action) -> bool : 任务通知
 * - task_wait_notify(timeout) -> uint32_t   : 等待任务通知
 *
 * 适配器模板 `Task` 会进行编译期验证，并转发所有调用。
 * 可选功能通过 SFINAE 暴露，未实现的策略不会产生相应代码。
 */

#pragma once

#ifndef STRATOS_KERNEL_TASK_HPP
#define STRATOS_KERNEL_TASK_HPP

#include "os_kernel/include/core/common_traits.hpp"
#include "os_kernel/include/core/types.hpp" // for KernelTypes
#include <cstddef>
#include <type_traits>

namespace strat_os::kernel::traits
{

// -----------------------------------------------------------------------------
// 任务管理器必需方法检测
// -----------------------------------------------------------------------------

/**
 * @brief 检测静态方法 create_task(entry, task, prio, stack_size)
 */
template <typename T, typename = void>
struct has_create_task_method : std::false_type {};
template <typename T>
struct has_create_task_method<
    T,
    std::void_t<decltype(T::create_task(std::declval<void (*)(void*)>(),
                                        std::declval<void*>(),
                                        std::declval<typename T::kernel_types_policy::priority_type>(),
                                        std::declval<std::size_t>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_create_task_method_v = has_create_task_method<T>::value;

/**
 * @brief 检测静态方法 destroy_task(tcb_type*)
 */
template <typename T, typename = void>
struct has_destroy_task_method : std::false_type {};
template <typename T>
struct has_destroy_task_method<T, std::void_t<decltype(T::destroy_task(std::declval<typename T::tcb_type*>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_destroy_task_method_v = has_destroy_task_method<T>::value;

/**
 * @brief 检测静态方法 suspend_task(tcb_type*)
 */
template <typename T, typename = void>
struct has_suspend_task_method : std::false_type {};
template <typename T>
struct has_suspend_task_method<T, std::void_t<decltype(T::suspend_task(std::declval<typename T::tcb_type*>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_suspend_task_method_v = has_suspend_task_method<T>::value;

/**
 * @brief 检测静态方法 resume_task(tcb_type*)
 */
template <typename T, typename = void>
struct has_resume_task_method : std::false_type {};
template <typename T>
struct has_resume_task_method<T, std::void_t<decltype(T::resume_task(std::declval<typename T::tcb_type*>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_resume_task_method_v = has_resume_task_method<T>::value;

/**
 * @brief 检测静态方法 set_priority(tcb_type*, priority_type)
 */
template <typename T, typename = void>
struct has_set_priority_method : std::false_type {};
template <typename T>
struct has_set_priority_method<
    T,
    std::void_t<decltype(T::set_priority(std::declval<typename T::tcb_type*>(),
                                         std::declval<typename T::kernel_types_policy::priority_type>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_set_priority_method_v = has_set_priority_method<T>::value;

// -----------------------------------------------------------------------------
// 可选方法检测（保持原有）
// -----------------------------------------------------------------------------

/**
 * @brief 检测静态方法 delay(tick_type)
 */
template <typename T, typename = void>
struct has_delay_method : std::false_type {};
template <typename T>
struct has_delay_method<T, std::void_t<decltype(T::delay(std::declval<typename T::kernel_types_policy::tick_type>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_delay_method_v = has_delay_method<T>::value;

/**
 * @brief 检测静态方法 delay_until(tick_type)
 */
template <typename T, typename = void>
struct has_delay_until_method : std::false_type {};
template <typename T>
struct has_delay_until_method<
    T,
    std::void_t<decltype(T::delay_until(std::declval<typename T::kernel_types_policy::tick_type>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_delay_until_method_v = has_delay_until_method<T>::value;

/**
 * @brief 检测静态方法 get_task_id(tcb_type*) -> task_id_type
 */
template <typename T, typename = void>
struct has_get_task_id_method : std::false_type {};
template <typename T>
struct has_get_task_id_method<T, std::void_t<decltype(T::get_task_id(std::declval<typename T::tcb_type*>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_get_task_id_method_v = has_get_task_id_method<T>::value;

/**
 * @brief 检测 get_task_id 返回类型是否为 task_id_type
 */
template <typename T, typename = void>
struct is_correct_get_task_id_return_type : std::false_type {};
template <typename T>
struct is_correct_get_task_id_return_type<T,
                                          std::void_t<decltype(T::get_task_id(std::declval<typename T::tcb_type*>()))>>
    : std::is_same<decltype(T::get_task_id(std::declval<typename T::tcb_type*>())),
                   typename T::kernel_types_policy::task_id_type> {};
template <typename T>
static constexpr bool is_correct_get_task_id_return_type_v = is_correct_get_task_id_return_type<T>::value;

/**
 * @brief 检测静态方法 get_task_state(tcb_type*) -> task_state_type
 */
template <typename T, typename = void>
struct has_get_task_state_method : std::false_type {};
template <typename T>
struct has_get_task_state_method<T, std::void_t<decltype(T::get_task_state(std::declval<typename T::tcb_type*>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_get_task_state_method_v = has_get_task_state_method<T>::value;

/**
 * @brief 检测 get_task_state 返回类型是否为 task_state_type
 */
template <typename T, typename = void>
struct is_correct_get_task_state_return_type : std::false_type {};
template <typename T>
struct is_correct_get_task_state_return_type<
    T,
    std::void_t<decltype(T::get_task_state(std::declval<typename T::tcb_type*>()))>>
    : std::is_same<decltype(T::get_task_state(std::declval<typename T::tcb_type*>())),
                   typename T::kernel_types_policy::task_state_type> {};
template <typename T>
static constexpr bool is_correct_get_task_state_return_type_v = is_correct_get_task_state_return_type<T>::value;

/**
 * @brief 检测静态方法 task_notify(tcb_type*, uint32_t, uint32_t) -> bool
 */
template <typename T, typename = void>
struct has_task_notify_method : std::false_type {};
template <typename T>
struct has_task_notify_method<T,
                              std::void_t<decltype(T::task_notify(std::declval<typename T::tcb_type*>(),
                                                                  std::declval<std::uint32_t>(),
                                                                  std::declval<std::uint32_t>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_task_notify_method_v = has_task_notify_method<T>::value;

/**
 * @brief 检测 task_notify 返回类型是否为 bool
 */
template <typename T, typename = void>
struct is_correct_task_notify_return_type : std::false_type {};
template <typename T>
struct is_correct_task_notify_return_type<T,
                                          std::void_t<decltype(T::task_notify(std::declval<typename T::tcb_type*>(),
                                                                              std::declval<std::uint32_t>(),
                                                                              std::declval<std::uint32_t>()))>>
    : std::is_same<bool,
                   decltype(T::task_notify(std::declval<typename T::tcb_type*>(),
                                           std::declval<std::uint32_t>(),
                                           std::declval<std::uint32_t>()))> {};
template <typename T>
static constexpr bool is_correct_task_notify_return_type_v = is_correct_task_notify_return_type<T>::value;

/**
 * @brief 检测静态方法 task_wait_notify(tick_type) -> uint32_t
 */
template <typename T, typename = void>
struct has_task_wait_notify_method : std::false_type {};
template <typename T>
struct has_task_wait_notify_method<
    T,
    std::void_t<decltype(T::task_wait_notify(std::declval<typename T::kernel_types_policy::tick_type>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_task_wait_notify_method_v = has_task_wait_notify_method<T>::value;

/**
 * @brief 检测 task_wait_notify 返回类型是否为 uint32_t
 */
template <typename T, typename = void>
struct is_correct_task_wait_notify_return_type : std::false_type {};
template <typename T>
struct is_correct_task_wait_notify_return_type<
    T,
    std::void_t<decltype(T::task_wait_notify(std::declval<typename T::kernel_types_policy::tick_type>()))>>
    : std::is_same<std::uint32_t,
                   decltype(T::task_wait_notify(std::declval<typename T::kernel_types_policy::tick_type>()))> {};
template <typename T>
static constexpr bool is_correct_task_wait_notify_return_type_v = is_correct_task_wait_notify_return_type<T>::value;

// -----------------------------------------------------------------------------
// 任务管理器策略有效性组合检测（复用公共萃取）
// -----------------------------------------------------------------------------

/**
 * @brief 组合检测，判断类型 T 是否为有效的任务管理器策略
 *
 * 要求 T 必须提供：
 * - 嵌套类型 kernel_types_policy, platform_context_policy, user_tcb_policy, tcb_type
 * - 静态方法 init(), create_task(), destroy_task(), suspend_task(), resume_task(), set_priority()
 * - tcb_type 必须是有效的 Tcb 实例化（通过 common_traits::is_valid_tcb_type 验证）
 */
template <typename T>
struct is_valid_task_policy : std::conjunction<has_kernel_types_policy_type<T>,     // 来自 common_traits
                                               has_platform_context_policy_type<T>, // 来自 common_traits
                                               has_user_tcb_policy_type<T>,         // 来自 common_traits
                                               has_tcb_type<T>,                     // 来自 common_traits
                                               has_init_method<T>,                  // 来自 common_traits
                                               has_create_task_method<T>,
                                               has_destroy_task_method<T>,
                                               has_suspend_task_method<T>,
                                               has_resume_task_method<T>,
                                               has_set_priority_method<T>,
                                               is_valid_tcb_type<T> // 来自 common_traits
                                               > {};

template <typename T>
static constexpr bool is_valid_task_policy_v = is_valid_task_policy<T>::value;

} // namespace strat_os::kernel::traits

namespace strat_os::kernel
{

/**
 * @brief 任务管理器适配器模板
 * @tparam TaskPolicy 具体的任务管理器策略类，必须满足 is_valid_task_policy
 *
 * 该类将任务管理策略包装为统一的静态接口，并进行编译期验证。
 * 所有必需方法均为内联且 noexcept，转发到策略类的对应静态方法。
 *
 * 可选方法（delay, delay_until, get_task_id, get_task_state, task_notify,
 * task_wait_notify）通过 SFINAE 条件暴露，未实现的策略不会产生相应代码。
 */
template <typename TaskPolicy, typename = std::enable_if_t<traits::is_valid_task_policy_v<TaskPolicy>>>
struct Task {
    /// 策略类型别名
    using Policy = TaskPolicy;

    /// 内核类型策略别名
    using kernel_types_policy = typename Policy::kernel_types_policy;
    /// 平台上下文策略别名
    using platform_context_policy = typename Policy::platform_context_policy;
    /// 用户 TCB 策略别名
    using user_tcb_policy = typename Policy::user_tcb_policy;
    /// TCB 类型别名
    using tcb_type = typename Policy::tcb_type;

    /// 内核数据类型
    using kernel_types = KernelTypes<kernel_types_policy>;
    /// 优先级类型别名
    using priority_type = typename kernel_types::priority_type;
    /// 时钟滴答类型别名
    using tick_type = typename kernel_types::tick_type;
    /// 任务 ID 类型别名
    using task_id_type = typename kernel_types::task_id_type;
    /// 任务状态类型别名
    using task_state_type = typename kernel_types::task_state_type;

    // ----- 细粒度静态断言，提供清晰的错误信息 -----
    static_assert(traits::has_kernel_types_policy_type_v<Policy>,
                  "TaskPolicy must provide nested type 'kernel_types_policy'");
    static_assert(traits::has_platform_context_policy_type_v<Policy>,
                  "TaskPolicy must provide nested type 'platform_context_policy'");
    static_assert(traits::has_user_tcb_policy_type_v<Policy>, "TaskPolicy must provide nested type 'user_tcb_policy'");
    static_assert(traits::has_tcb_type_v<Policy>, "TaskPolicy must provide nested type 'tcb_type'");
    static_assert(traits::has_init_method_v<Policy>, "TaskPolicy must provide init()");
    static_assert(traits::has_create_task_method_v<Policy>,
                  "TaskPolicy must provide create_task(entry, task, prio, stack_size)");
    static_assert(traits::has_destroy_task_method_v<Policy>, "TaskPolicy must provide destroy_task(tcb_type*)");
    static_assert(traits::has_suspend_task_method_v<Policy>, "TaskPolicy must provide suspend_task(tcb_type*)");
    static_assert(traits::has_resume_task_method_v<Policy>, "TaskPolicy must provide resume_task(tcb_type*)");
    static_assert(traits::has_set_priority_method_v<Policy>,
                  "TaskPolicy must provide set_priority(tcb_type*, priority_type)");
    static_assert(traits::is_valid_tcb_type_v<Policy>,
                  "Policy::tcb_type must be Tcb<kernel_types_policy, platform_context_policy, user_tcb_policy>");

    // ----- 必需方法（所有任务管理器必须实现）-----

    /**
     * @brief 初始化任务管理器（通常创建 idle 任务并设置调度器的空闲任务）
     */
    inline static void init() noexcept {
        Policy::init();
    }

    /**
     * @brief 创建一个新任务
     * @param entry      任务入口函数（参数为 void* 的任务对象指针）
     * @param task       任务对象指针（传递给入口函数）
     * @param prio       任务优先级
     * @param stack_size 任务栈大小（字节）
     * @return 新任务的 TCB 指针，失败返回 nullptr
     */
    [[nodiscard]] inline static tcb_type* create_task(void (*entry)(void*),
                                                      void* task,
                                                      priority_type prio,
                                                      std::size_t stack_size) noexcept {
        return Policy::create_task(entry, task, prio, stack_size);
    }

    /**
     * @brief 删除任务（任务必须处于终止状态或强制终止）
     * @param tcb 要删除的任务的 TCB 指针
     */
    inline static void destroy_task(tcb_type* tcb) noexcept {
        Policy::destroy_task(tcb);
    }

    /**
     * @brief 挂起任务（任务不再参与调度）
     * @param tcb 要挂起的任务
     */
    inline static void suspend_task(tcb_type* tcb) noexcept {
        Policy::suspend_task(tcb);
    }

    /**
     * @brief 恢复挂起的任务
     * @param tcb 要恢复的任务
     */
    inline static void resume_task(tcb_type* tcb) noexcept {
        Policy::resume_task(tcb);
    }

    /**
     * @brief 动态改变任务优先级
     * @param tcb      目标任务
     * @param new_prio 新优先级
     */
    inline static void set_priority(tcb_type* tcb, priority_type new_prio) noexcept {
        Policy::set_priority(tcb, new_prio);
    }

    // ----- 可选方法（通过 SFINAE 暴露）-----

    /**
     * @brief 使当前任务进入延时状态（相对节拍数）
     * @param ticks 延时节拍数
     * @note 仅当策略提供 delay() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_delay_method_v<P>>>
    inline static void delay(tick_type ticks) noexcept {
        P::delay(ticks);
    }

    /**
     * @brief 使当前任务延时到绝对节拍数（用于周期性任务）
     * @param absolute_ticks 绝对节拍数
     * @note 仅当策略提供 delay_until() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_delay_until_method_v<P>>>
    inline static void delay_until(tick_type absolute_ticks) noexcept {
        P::delay_until(absolute_ticks);
    }

    /**
     * @brief 获取任务唯一标识符
     * @param tcb 目标任务
     * @return 任务 ID
     * @note 仅当策略提供 get_task_id() 且返回类型正确时可用
     */
    template <typename P = Policy,
              typename   = std::enable_if_t<traits::has_get_task_id_method_v<P> &&
                                            traits::is_correct_get_task_id_return_type_v<P>>>
    [[nodiscard]] inline static task_id_type get_task_id(tcb_type* tcb) noexcept {
        static_assert(traits::is_correct_get_task_id_return_type_v<P>, "get_task_id() must return task_id_type");
        return P::get_task_id(tcb);
    }

    /**
     * @brief 获取任务当前状态
     * @param tcb 目标任务
     * @return 任务状态枚举
     * @note 仅当策略提供 get_task_state() 且返回类型正确时可用
     */
    template <typename P = Policy,
              typename   = std::enable_if_t<traits::has_get_task_state_method_v<P> &&
                                            traits::is_correct_get_task_state_return_type_v<P>>>
    [[nodiscard]] inline static task_state_type get_task_state(tcb_type* tcb) noexcept {
        static_assert(traits::is_correct_get_task_state_return_type_v<P>,
                      "get_task_state() must return task_state_type");
        return P::get_task_state(tcb);
    }

    /**
     * @brief 向指定任务发送通知（轻量级任务间信号）
     * @param tcb    目标任务
     * @param value  通知值
     * @param action 通知动作（如设置、加一、覆盖等，由策略定义）
     * @return 是否成功（如目标任务未等待通知等）
     * @note 仅当策略提供 task_notify() 且返回 bool 时可用
     */
    template <typename P = Policy,
              typename   = std::enable_if_t<traits::has_task_notify_method_v<P> &&
                                            traits::is_correct_task_notify_return_type_v<P>>>
    [[nodiscard]] inline static bool task_notify(tcb_type* tcb, std::uint32_t value, std::uint32_t action) noexcept {
        static_assert(traits::is_correct_task_notify_return_type_v<P>, "task_notify() must return bool");
        return P::task_notify(tcb, value, action);
    }

    /**
     * @brief 当前任务等待通知（可带超时）
     * @param timeout 超时节拍数（0 表示无限等待）
     * @return 收到的通知值
     * @note 仅当策略提供 task_wait_notify() 且返回 uint32_t 时可用
     */
    template <typename P = Policy,
              typename   = std::enable_if_t<traits::has_task_wait_notify_method_v<P> &&
                                            traits::is_correct_task_wait_notify_return_type_v<P>>>
    [[nodiscard]] inline static std::uint32_t task_wait_notify(tick_type timeout) noexcept {
        static_assert(traits::is_correct_task_wait_notify_return_type_v<P>, "task_wait_notify() must return uint32_t");
        return P::task_wait_notify(timeout);
    }
};

} // namespace strat_os::kernel

#endif // STRATOS_KERNEL_TASK_HPP