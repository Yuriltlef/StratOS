/**
 * @file interrupt.hpp
 * @author StratOS Team
 * @brief 中断控制器策略接口与适配器
 * @version 1.0.0
 * @date 2026-03-27
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了硬件抽象层（HAL）的中断控制器抽象接口。
 * 它采用静态策略模式，通过 traits 编译期检测策略类是否满足接口约定，
 * 并提供适配器模板将策略类包装为统一的静态接口，供 RTOS 内核使用。
 *
 * 主要组件：
 * - traits 命名空间：提供类型萃取，用于验证策略类是否符合接口要求。
 * - InterruptController 适配器模板：接收一个策略类，静态断言其合法性，
 *   并转发所有中断控制操作。
 *
 * 该设计保证了零开销抽象，所有函数均为内联且 noexcept，适合嵌入式高安全环境。
 */
#pragma once

#ifndef STRATOS_HAL_INTERRUPT_HPP
#define STRATOS_HAL_INTERRUPT_HPP

#include <type_traits> // for std::false_type, std::true_type, etc.
#include <utility>     // for std::declval

namespace strat_os::hal::traits
{
/**
 * @brief 检测类型 T 是否包含嵌套类型 IRQn_Type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_irqn_type : std::false_type {};
template <typename T>
struct has_irqn_type<T, std::void_t<typename T::IRQn_Type>> : std::true_type {};
template <typename T>
inline constexpr bool has_irqn_type_v = has_irqn_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 priority_group_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_priority_group_type : std::false_type {};
template <typename T>
struct has_priority_group_type<T, std::void_t<typename T::priority_group_type>> : std::true_type {};
template <typename T>
inline constexpr bool has_priority_group_type_v = has_priority_group_type<T>::value;

/**
 * @brief 检测类型 T 的 priority_group_type 是否为无符号整数类型
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_valid_priority_group_type : std::is_unsigned<typename T::priority_group_type> {};
template <typename T>
inline constexpr bool is_valid_priority_group_type_v = is_valid_priority_group_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 priority_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_priority_type : std::false_type {};
template <typename T>
struct has_priority_type<T, std::void_t<typename T::priority_type>> : std::true_type {};
template <typename T>
inline constexpr bool has_priority_type_v = has_priority_type<T>::value;

/**
 * @brief 检测类型 T 的 priority_type 是否为无符号整数类型
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_valid_priority_type : std::is_unsigned<typename T::priority_type> {};
template <typename T>
inline constexpr bool is_valid_priority_type_v = is_valid_priority_type<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 enable(IRQn_Type)
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_enable_method : std::false_type {};
template <typename T>
struct has_enable_method<T, std::void_t<decltype(T::enable(std::declval<typename T::IRQn_Type>()))>> : std::true_type {
};
template <typename T>
inline constexpr bool has_enable_method_v = has_enable_method<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 disable(IRQn_Type)
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_disable_method : std::false_type {};
template <typename T>
struct has_disable_method<T, std::void_t<decltype(T::disable(std::declval<typename T::IRQn_Type>()))>>
    : std::true_type {};
template <typename T>
inline constexpr bool has_disable_method_v = has_disable_method<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 set_priority(IRQn_Type, T::priority_group_type)
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_set_priority_method : std::false_type {};
template <typename T>
struct has_set_priority_method<T,
                               std::void_t<decltype(T::set_priority(std::declval<typename T::IRQn_Type>(),
                                                                    std::declval<typename T::priority_group_type>()))>>
    : std::true_type {};
template <typename T>
inline constexpr bool has_set_priority_method_v = has_set_priority_method<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 get_priority(IRQn_Type)
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_get_priority_method : std::false_type {};
template <typename T>
struct has_get_priority_method<T, std::void_t<decltype(T::get_priority(std::declval<typename T::IRQn_Type>()))>>
    : std::true_type {};
template <typename T>
inline constexpr bool has_get_priority_method_v = has_get_priority_method<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 trigger_software(IRQn_Type)
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_trigger_software_method : std::false_type {};
template <typename T>
struct has_trigger_software_method<T, std::void_t<decltype(T::trigger_software(std::declval<typename T::IRQn_Type>()))>>
    : std::true_type {};
template <typename T>
inline constexpr bool has_trigger_software_method_v = has_trigger_software_method<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 global_enable()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_global_enable_method : std::false_type {};
template <typename T>
struct has_global_enable_method<T, std::void_t<decltype(T::global_enable())>> : std::true_type {};
template <typename T>
inline constexpr bool has_global_enable_method_v = has_global_enable_method<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 global_disable()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_global_disable_method : std::false_type {};
template <typename T>
struct has_global_disable_method<T, std::void_t<decltype(T::global_disable())>> : std::true_type {};
template <typename T>
inline constexpr bool has_global_disable_method_v = has_global_disable_method<T>::value;

/**
 * @brief 组合检测：判断类型 T 是否为有效的中断控制器策略
 * @tparam T 待检测的类型
 *
 * 要求 T 必须提供 IRQn_Type 嵌套类型以及上述所有静态方法。
 */
template <typename T>
struct is_valid_interrupt_controller : std::conjunction<has_irqn_type<T>,
                                                        has_priority_group_type<T>,
                                                        is_valid_priority_group_type<T>,
                                                        has_priority_type<T>,
                                                        is_valid_priority_type<T>,
                                                        has_enable_method<T>,
                                                        has_disable_method<T>,
                                                        has_set_priority_method<T>,
                                                        has_get_priority_method<T>,
                                                        has_trigger_software_method<T>,
                                                        has_global_enable_method<T>,
                                                        has_global_disable_method<T>> {};
template <typename T>
inline constexpr bool is_valid_interrupt_controller_v = is_valid_interrupt_controller<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 in_isr()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_in_isr_method : std::false_type {};
template <typename T>
struct has_in_isr_method<T, std::void_t<decltype(T::in_isr())>> : std::true_type {};
template <typename T>
inline constexpr bool has_in_isr_method_v = has_in_isr_method<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 get_current_irq()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_get_current_irq_method : std::false_type {};
template <typename T>
struct has_get_current_irq_method<T, std::void_t<decltype(T::get_current_irq())>> : std::true_type {};
template <typename T>
inline constexpr bool has_get_current_irq_method_v = has_get_current_irq_method<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 set_priority_grouping(T::priority_group_type)
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_set_priority_grouping_method : std::false_type {};
template <typename T>
struct has_set_priority_grouping_method<
    T,
    std::void_t<decltype(T::set_priority_grouping(std::declval<typename T::priority_group_type>()))>> : std::true_type {
};
template <typename T>
inline constexpr bool has_set_priority_grouping_method_v = has_set_priority_grouping_method<T>::value;
/**
 * @brief 检测类型 T 是否提供静态方法 in_isr()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_get_priority_grouping_method : std::false_type {};
template <typename T>
struct has_get_priority_grouping_method<T, std::void_t<decltype(T::get_priority_grouping())>> : std::true_type {};
template <typename T>
inline constexpr bool has_get_priority_grouping_method_v = has_get_priority_grouping_method<T>::value;

/**
 * @brief 组合检测：判断类型 T 是否为增强的中断控制器策略
 * @tparam T 待检测的类型
 *
 * 要求 T 必须提供上述所有增强静态方法（in_isr、get_current_irq、set_priority_grouping、get_priority_grouping）。
 */
template <typename T>
struct is_enhanced_interrupt_controller : std::conjunction<has_in_isr_method<T>,
                                                           has_get_current_irq_method<T>,
                                                           has_set_priority_grouping_method<T>,
                                                           has_get_priority_grouping_method<T>> {};
template <typename T>
inline constexpr bool is_enhanced_interrupt_controller_v = is_enhanced_interrupt_controller<T>::value;

} // namespace strat_os::hal::traits

namespace strat_os::hal
{
/**
 * @brief 中断控制器适配器模板
 * @tparam InterruptControllerPolicy 具体的策略类，必须满足中断控制器接口
 *
 * 该类将策略类包装为统一的静态接口，并进行编译期验证。
 * 所有方法均为内联且 noexcept，转发到策略类的对应静态方法。
 *
 * 使用示例：
 * @code
 * using MyInterrupt = InterruptController<CortexM3InterruptController>;
 * MyInterrupt::global_disable();
 * MyInterrupt::enable(IRQn_USART1);
 * @endcode
 */
template <typename InterruptControllerPolicy,
          typename = std::enable_if_t<traits::is_valid_interrupt_controller_v<InterruptControllerPolicy>>>
struct InterruptController {
    /// 策略类别名
    using Policy = InterruptControllerPolicy;

    /// 细粒度静态断言，提供清晰的错误信息
    static_assert(traits::has_irqn_type_v<Policy>, "Policy must define IRQn_Type");
    static_assert(traits::has_priority_group_type_v<Policy>, "Policy must define priority_group_type");
    static_assert(traits::is_valid_priority_group_type_v<Policy>, "Policy must define a valid priority_group_type");
    static_assert(traits::has_priority_type_v<Policy>, "Policy must define priority_type");
    static_assert(traits::is_valid_priority_type_v<Policy>, "Policy must define a valid priority_type");
    static_assert(traits::has_enable_method_v<Policy>, "Policy must provide enable(IRQn_Type)");
    static_assert(traits::has_disable_method_v<Policy>, "Policy must provide disable(IRQn_Type)");
    static_assert(traits::has_set_priority_method_v<Policy>, "Policy must provide set_priority(IRQn_Type, uint32_t)");
    static_assert(traits::has_get_priority_method_v<Policy>, "Policy must provide get_priority(IRQn_Type)");
    static_assert(traits::has_trigger_software_method_v<Policy>, "Policy must provide trigger_software(IRQn_Type)");
    static_assert(traits::has_global_enable_method_v<Policy>, "Policy must provide global_enable()");
    static_assert(traits::has_global_disable_method_v<Policy>, "Policy must provide global_disable()");

    /// 中断号类型，取自策略类
    using IRQn_Type           = typename Policy::IRQn_Type;
    using priority_group_type = typename Policy::priority_group_type;
    using priority_type       = typename Policy::priority_type;
    /// 是否支持增强功能（编译期常量）
    static constexpr bool enhanced_controller = traits::is_enhanced_interrupt_controller_v<Policy>;

    /**
     * @brief 使能指定中断
     * @param irq 中断号
     */
    inline static void enable(IRQn_Type irq) noexcept {
        Policy::enable(irq);
    }

    /**
     * @brief 禁用指定中断
     * @param irq 中断号
     */
    inline static void disable(IRQn_Type irq) noexcept {
        Policy::disable(irq);
    }

    /**
     * @brief 设置中断优先级
     * @param irq      中断号
     * @param priority 优先级值（数值越小优先级越高）
     */
    inline static void set_priority(IRQn_Type irq, priority_type priority) noexcept {
        Policy::set_priority(irq, priority);
    }

    /**
     * @brief 获取中断优先级
     * @param irq 中断号
     * @return 优先级值（类型与策略类返回一致）
     */
    [[nodiscard]] inline static auto get_priority(IRQn_Type irq) noexcept -> decltype(Policy::get_priority(irq)) {
        return Policy::get_priority(irq);
    }

    /**
     * @brief 触发软件中断（如 PendSV）
     * @param irq 中断号
     */
    inline static void trigger_software(IRQn_Type irq) noexcept {
        Policy::trigger_software(irq);
    }

    /**
     * @brief 全局使能所有可屏蔽中断
     */
    inline static void global_enable() noexcept {
        Policy::global_enable();
    }

    /**
     * @brief 全局禁用所有可屏蔽中断
     */
    inline static void global_disable() noexcept {
        Policy::global_disable();
    }

    // ----- 可选扩展方法（若策略提供则转发，否则不编译） -----

    /**
     * @brief 判断当前是否在中断服务程序中
     * @return true 在中断中，false 在任务中
     * @note 仅当策略类提供 in_isr() 方法时可用，否则调用会导致编译错误
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_in_isr_method_v<P>>>
    [[nodiscard]] static bool in_isr() noexcept {
        return P::in_isr();
    }

    /**
     * @brief 获取当前正在执行的中断号
     * @return 中断号，若不在中断中则返回特定值（如 -1）
     * @note 仅当策略类提供 get_current_irq() 方法时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_get_current_irq_method_v<P>>>
    [[nodiscard]] static auto get_current_irq() noexcept -> decltype(P::get_current_irq()) {
        return P::get_current_irq();
    }

    /**
     * @brief 设置中断优先级分组（Cortex-M 特有）
     * @param group 优先级分组值
     * @note 仅当策略类提供 set_priority_grouping() 方法时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_set_priority_grouping_method_v<P>>>
    static void set_priority_grouping(priority_group_type group) noexcept {
        P::set_priority_grouping(group);
    }

    /**
     * @brief 获取中断优先级分组
     * @return 优先级分组值
     * @note 仅当策略类提供 get_priority_grouping() 方法时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_get_priority_grouping_method_v<P>>>
    [[nodiscard]] static auto get_priority_grouping() noexcept -> decltype(P::get_priority_grouping()) {
        return P::get_priority_grouping();
    }
};
} // namespace strat_os::hal

#endif // STRATOS_HAL_INTERRUPT_HPP