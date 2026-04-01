/**
 * @file system_tick.hpp
 * @author StratOS Team
 * @brief 系统节拍定时器（SystemTick）策略接口与适配器
 * @version 1.0.0
 * @date 2026-03-31
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了硬件抽象层（HAL）的 SystemTick 定时器抽象接口，采用静态策略模式。
 * SystemTick 是一个简单的 n 位递减计数器，通常用于 RTOS 的系统时钟节拍。
 *
 * 该设计将 SystemTick 的硬件操作（初始化、使能/禁用、中断控制、状态读取等）
 * 封装为策略类，并通过类型萃取在编译期验证策略的完整性，最终通过适配器
 * 模板 `SystemTick` 提供统一的静态接口供 RTOS 内核使用。
 *
 * 主要特性：
 * - 策略类必须定义 `reload_type`（重装载值类型）和 `clock_source_type`（时钟源类型）
 * - 提供可选增强功能（如读取校准值），通过 SFINAE 自动检测并暴露接口
 * - 所有方法均为内联且 noexcept，零开销抽象
 *
 * @note 典型的重装载值类型为 uint32_t，但实际硬件可能只使用低 24 位。
 *       时钟源类型可以是枚举或整数，由具体策略定义。
 *
 * @warning SystemTick 的重装载值必须遵循平台规范，否则硬件行为未定义。
 */
#pragma once

#ifndef STRATOS_HAL_SYSTEM_TICK_HPP
#define STRATOS_HAL_SYSTEM_TICK_HPP

#include "os_hal/include/common_traits.hpp" // for enable/disable method traits
#include <type_traits>                      // for std::false_type, std::true_type, etc.
#include <utility>                          // for std::declval

namespace strat_os::hal::traits
{

/**
 * @brief 检测类型 T 是否包含嵌套类型 reload_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_reload_type : std::false_type {};
template <typename T>
struct has_reload_type<T, std::void_t<typename T::reload_type>> : std::true_type {};
template <typename T>
inline constexpr bool has_reload_type_v = has_reload_type<T>::value;

/**
 * @brief 检测类型 T 的 reload_type 是否为无符号整数类型
 * @tparam T 待检测的类型，仅当 T 包含 reload_type 时使用
 */
template <typename T, typename = void>
struct is_valid_reload_type : std::false_type {};
template <typename T>
struct is_valid_reload_type<T, std::void_t<typename T::reload_type>> : std::is_unsigned<typename T::reload_type> {};
template <typename T>
inline constexpr bool is_valid_reload_type_v = is_valid_reload_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 clock_source_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_clock_source_type : std::false_type {};
template <typename T>
struct has_clock_source_type<T, std::void_t<typename T::clock_source_type>> : std::true_type {};
template <typename T>
inline constexpr bool has_clock_source_type_v = has_clock_source_type<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 init(reload_type, clock_source_type)
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_init_method : std::false_type {};
template <typename T>
struct has_init_method<T,
                       std::void_t<decltype(T::init(std::declval<typename T::reload_type>(),
                                                    std::declval<typename T::clock_source_type>()))>> : std::true_type {
};
template <typename T>
inline constexpr bool has_init_method_v = has_init_method<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 enable_irq()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_enable_irq_method : std::false_type {};
template <typename T>
struct has_enable_irq_method<T, std::void_t<decltype(T::enable_irq())>> : std::true_type {};
template <typename T>
inline constexpr bool has_enable_irq_method_v = has_enable_irq_method<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 disable_irq()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_disable_irq_method : std::false_type {};
template <typename T>
struct has_disable_irq_method<T, std::void_t<decltype(T::disable_irq())>> : std::true_type {};
template <typename T>
inline constexpr bool has_disable_irq_method_v = has_disable_irq_method<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 get_value() -> reload_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_get_value_method : std::false_type {};
template <typename T>
struct has_get_value_method<T, std::void_t<decltype(T::get_value())>> : std::true_type {};
template <typename T>
inline constexpr bool has_get_value_method_v = has_get_value_method<T>::value;

/**
 * @brief 检测类型 T 的 get_value() 返回类型是否为 reload_type
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_correct_get_value_return_type : std::is_same<decltype(T::get_value()), typename T::reload_type> {};
template <typename T>
inline constexpr bool is_correct_get_value_return_type_v = is_correct_get_value_return_type<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 is_overflow()（返回 bool）
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_is_overflow_method : std::false_type {};
template <typename T>
struct has_is_overflow_method<T, std::void_t<decltype(T::is_overflow())>> : std::true_type {};
template <typename T>
inline constexpr bool has_is_overflow_method_v = has_is_overflow_method<T>::value;

/**
 * @brief 检测 is_overflow() 返回类型是否与 bool 一致
 */
template <typename T>
struct is_correct_is_overflow_return_type : std::is_same<decltype(T::is_overflow()), bool> {};
template <typename T>
inline constexpr bool is_correct_is_overflow_return_type_v = is_correct_is_overflow_return_type<T>::value;

/**
 * @brief 组合检测，判断类型 T 是否为有效的 SystemTick 策略
 * @tparam T 待检测的类型
 *
 * 要求 T 必须定义 reload_type 和 clock_source_type，并提供以下静态方法：
 * - init(reload_type, clock_source_type)
 * - enable()
 * - disable()
 * - enable_irq()
 * - disable_irq()
 * - get_value() -> reload_type
 * - is_overflow() -> bool
 */
template <typename T>
struct is_valid_systick_policy : std::conjunction<has_reload_type<T>,
                                                  is_valid_reload_type<T>,
                                                  has_clock_source_type<T>,
                                                  has_init_method<T>,
                                                  has_enable_method<T>,
                                                  has_disable_method<T>,
                                                  has_enable_irq_method<T>,
                                                  has_disable_irq_method<T>,
                                                  has_get_value_method<T>,
                                                  is_correct_get_value_return_type<T>,
                                                  has_is_overflow_method<T>,
                                                  is_correct_is_overflow_return_type<T>> {};
template <typename T>
inline constexpr bool is_valid_systick_policy_v = is_valid_systick_policy<T>::value;

/// 增强功能检测
/**
 * @brief 检测类型 T 是否提供静态方法 get_calibration()（返回任意类型）
 * @tparam T 待检测的类型
 * @note 该方法是可选的，用于读取 SystemTick 校准值。
 */
template <typename T, typename = void>
struct has_get_calibration_method : std::false_type {};
template <typename T>
struct has_get_calibration_method<T, std::void_t<decltype(T::get_calibration())>> : std::true_type {};
template <typename T>
inline constexpr bool has_get_calibration_method_v = has_get_calibration_method<T>::value;

} // namespace strat_os::hal::traits

namespace strat_os::hal
{

/**
 * @brief SystemTick 定时器适配器模板
 * @tparam SystemTickPolicy 具体的策略类，必须满足 SystemTick 策略接口
 *
 * 该类将策略类包装为统一的静态接口，并进行编译期验证。
 * 所有方法均为内联且 noexcept，转发到策略类的对应静态方法。
 *
 * 若策略支持可选功能（如 get_calibration），则会通过 SFINAE 提供相应重载；
 * 否则这些方法在适配器中不可用，避免代码膨胀。
 *
 * @par 使用示例
 * @code
 * // 假设 CortexM3SysTick 已实现所有必需方法
 * using MySysTick = SystemTick<CortexM3SysTick>;
 *
 * // 初始化：重装载值 1000，时钟源选择处理器时钟
 * MySysTick::init(1000, CortexM3SysTick::clock_source::processor);
 *
 * // 使能定时器和中断
 * MySysTick::enable();
 * MySysTick::enable_irq();
 *
 * // 读取当前计数值
 * auto val = MySysTick::get_value();
 *
 * // 检查是否溢出
 * if (MySysTick::is_overflow()) {
 *     // 处理
 * }
 * @endcode
 *
 * @note 策略类必须定义 reload_type 和 clock_source_type 嵌套类型。
 * @warning 重装载值不能超过硬件支持的最大值（如 Cortex‑M 为 24 位，即 0xFFFFFF）。
 */
template <typename SystemTickPolicy, typename = std::enable_if_t<traits::is_valid_systick_policy_v<SystemTickPolicy>>>
struct SystemTick {
    /// 策略类别名
    using Policy = SystemTickPolicy;

    // ----- 细粒度静态断言，提供清晰的错误信息 -----
    static_assert(traits::has_reload_type_v<Policy>, "Policy must define 'reload_type'");
    static_assert(traits::is_valid_reload_type_v<Policy>, "Policy's 'reload_type' must be an unsigned integer");
    static_assert(traits::has_clock_source_type_v<Policy>, "Policy must define 'clock_source_type'");
    static_assert(traits::has_init_method_v<Policy>, "Policy must provide init(reload_type, clock_source_type)");
    static_assert(traits::has_enable_method_v<Policy>, "Policy must provide enable()");
    static_assert(traits::has_disable_method_v<Policy>, "Policy must provide disable()");
    static_assert(traits::has_enable_irq_method_v<Policy>, "Policy must provide enable_irq()");
    static_assert(traits::has_disable_irq_method_v<Policy>, "Policy must provide disable_irq()");
    static_assert(traits::has_get_value_method_v<Policy>, "Policy must provide get_value() -> reload_type");
    static_assert(traits::is_correct_get_value_return_type_v<Policy>,
                  "Policy's get_value() must return the correct reload_type");
    static_assert(traits::has_is_overflow_method_v<Policy>, "Policy must provide is_overflow() -> bool");
    static_assert(traits::is_correct_is_overflow_return_type_v<Policy>, "Policy's is_overflow() must return bool");

    /// 重装载值类型别名
    using reload_type = typename Policy::reload_type;
    /// 时钟源类型别名
    using clock_source_type = typename Policy::clock_source_type;

    /// 是否支持增强功能（校准值读取）
    static constexpr bool enhanced_systick = traits::has_get_calibration_method_v<Policy>;

    /**
     * @brief 初始化 SystemTick 定时器
     * @param reload 重装载值（通常为 24 位，实际有效位数取决于硬件）
     * @param src    时钟源（具体值由策略定义）
     *
     * @note 通常此方法会配置重装载寄存器、时钟源，并可能使能计数器。
     *       但不会自动使能中断，需单独调用 enable_irq()。
     */
    inline static void init(reload_type reload, clock_source_type src) noexcept {
        Policy::init(reload, src);
    }

    /**
     * @brief 使能 SystemTick 计数器
     */
    inline static void enable() noexcept {
        Policy::enable();
    }

    /**
     * @brief 禁用 SystemTick 计数器
     */
    inline static void disable() noexcept {
        Policy::disable();
    }

    /**
     * @brief 使能 SystemTick 中断
     * @note 当计数器减到 0 时触发中断。
     */
    inline static void enable_irq() noexcept {
        Policy::enable_irq();
    }

    /**
     * @brief 禁用 SystemTick 中断
     */
    inline static void disable_irq() noexcept {
        Policy::disable_irq();
    }

    /**
     * @brief 获取当前计数值
     * @return 当前计数值（递减中）
     */
    inline static reload_type get_value() noexcept {
        return Policy::get_value();
    }

    /**
     * @brief 检查是否发生了溢出
     * @return true 如果计数器从 1 减到 0（即发生了溢出）
     * @note 读取该标志后，硬件通常会自动清除该位。
     */
    inline static bool is_overflow() noexcept {
        return Policy::is_overflow();
    }

    // ----- 可选增强功能 -----

    /**
     * @brief 获取 SystemTick 校准值
     * @return 校准值（类型由策略定义，通常为 uint32_t）
     * @note 仅当策略提供 get_calibration() 方法时可用。
     *       校准值可用于生成精确的延时（如 10ms）。
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_get_calibration_method_v<P>>>
    inline static auto get_calibration() noexcept -> decltype(P::get_calibration()) {
        return Policy::get_calibration();
    }
};

} // namespace strat_os::hal

#endif // STRATOS_HAL_SYSTEM_TICK_HPP