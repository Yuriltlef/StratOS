/**
 * @file debug.hpp
 * @author StratOS Team
 * @brief 调试策略接口与适配器
 * @version 1.0.0
 * @date 2026-03-31
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了硬件抽象层（HAL）的调试抽象接口，采用静态策略模式。
 * 它将调试相关的硬件操作（周期计数器、断点、ITM 输出等）封装为策略类，
 * 并通过类型萃取在编译期检测策略类的能力，最终通过适配器模板 `Debug`
 * 提供统一的静态接口供 RTOS 内核和应用使用。
 *
 * 调试功能分为必需和可选两类：
 * - 必需功能：断点、周期计数器（性能测量）——所有平台必须实现。
 * - 可选功能：ITM 软件跟踪（`send_char`/`send_block`）——由平台选择性实现，
 *   适配器通过 SFINAE 自动暴露相应接口。
 *
 * 该设计保证了零开销抽象，所有方法均为内联且 noexcept，适合嵌入式高安全环境。
 */
#pragma once

#ifndef STRATOS_HAL_DEBUG_HPP
#define STRATOS_HAL_DEBUG_HPP

#include <cstddef>     // for std::size_t, std::byte
#include <type_traits> // for std::false_type, std::true_type, etc.
#include <utility>     // for std::declval

namespace strat_os::hal::traits
{

// -----------------------------------------------------------------------------
// 周期计数器相关类型检测
// -----------------------------------------------------------------------------

/**
 * @brief 检测类型 T 是否包含嵌套类型 cycle_counter_size_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_cycle_counter_size_type : std::false_type {};
template <typename T>
struct has_cycle_counter_size_type<T, std::void_t<typename T::cycle_counter_size_type>> : std::true_type {};
template <typename T>
static constexpr bool has_cycle_counter_size_type_v = has_cycle_counter_size_type<T>::value;

/**
 * @brief 检测 cycle_counter_size_type 是否为无符号整数类型
 * @tparam T 待检测的类型，仅当 T 包含 cycle_counter_size_type 时使用
 */
template <typename T, typename = void>
struct is_valid_cycle_counter_size_type : std::false_type {};
template <typename T>
struct is_valid_cycle_counter_size_type<T, std::void_t<typename T::cycle_counter_size_type>>
    : std::is_unsigned<typename T::cycle_counter_size_type> {};
template <typename T>
static constexpr bool is_valid_cycle_counter_size_type_v = is_valid_cycle_counter_size_type<T>::value;

// -----------------------------------------------------------------------------
// 必需方法检测
// -----------------------------------------------------------------------------

/**
 * @brief 检测静态方法 bkpt()
 */
template <typename T, typename = void>
struct has_bkpt_method : std::false_type {};
template <typename T>
struct has_bkpt_method<T, std::void_t<decltype(T::bkpt())>> : std::true_type {};
template <typename T>
static constexpr bool has_bkpt_method_v = has_bkpt_method<T>::value;

/**
 * @brief 检测静态方法 enable_cycle_counter()
 */
template <typename T, typename = void>
struct has_enable_cycle_counter_method : std::false_type {};
template <typename T>
struct has_enable_cycle_counter_method<T, std::void_t<decltype(T::enable_cycle_counter())>> : std::true_type {};
template <typename T>
static constexpr bool has_enable_cycle_counter_method_v = has_enable_cycle_counter_method<T>::value;

/**
 * @brief 检测静态方法 disable_cycle_counter()
 */
template <typename T, typename = void>
struct has_disable_cycle_counter_method : std::false_type {};
template <typename T>
struct has_disable_cycle_counter_method<T, std::void_t<decltype(T::disable_cycle_counter())>> : std::true_type {};
template <typename T>
static constexpr bool has_disable_cycle_counter_method_v = has_disable_cycle_counter_method<T>::value;

/**
 * @brief 检测静态方法 get_cycle_counter()
 */
template <typename T, typename = void>
struct has_get_cycle_counter_method : std::false_type {};
template <typename T>
struct has_get_cycle_counter_method<T, std::void_t<decltype(T::get_cycle_counter())>> : std::true_type {};
template <typename T>
static constexpr bool has_get_cycle_counter_method_v = has_get_cycle_counter_method<T>::value;

/**
 * @brief 检测静态方法 is_cycle_counter_enabled()
 */
template <typename T, typename = void>
struct has_is_cycle_counter_enabled_method : std::false_type {};
template <typename T>
struct has_is_cycle_counter_enabled_method<T, std::void_t<decltype(T::is_cycle_counter_enabled())>> : std::true_type {};
template <typename T>
static constexpr bool has_is_cycle_counter_enabled_method_v = has_is_cycle_counter_enabled_method<T>::value;

/**
 * @brief 检测 get_cycle_counter() 的返回类型是否与 cycle_counter_size_type 严格一致
 */
template <typename T, typename = void>
struct is_correct_cycle_counter_return_type : std::false_type {};
template <typename T>
struct is_correct_cycle_counter_return_type<T, std::void_t<decltype(T::get_cycle_counter())>>
    : std::is_same<decltype(T::get_cycle_counter()), typename T::cycle_counter_size_type> {};
template <typename T>
static constexpr bool is_correct_cycle_counter_return_type_v = is_correct_cycle_counter_return_type<T>::value;

/**
 * @brief 检测静态方法 is_cycle_counter_enabled() 的返回类型是否为 bool
 */
template <typename T, typename = void>
struct is_correct_is_cycle_counter_enabled_return_type : std::false_type {};
template <typename T>
struct is_correct_is_cycle_counter_enabled_return_type<T, std::void_t<decltype(T::is_cycle_counter_enabled())>>
    : std::is_same<decltype(T::is_cycle_counter_enabled()), bool> {};
template <typename T>
static constexpr bool is_correct_is_cycle_counter_enabled_return_type_v =
    is_correct_is_cycle_counter_enabled_return_type<T>::value;

// -----------------------------------------------------------------------------
// 组合检测：是否为有效的调试策略
// -----------------------------------------------------------------------------

/**
 * @brief 组合检测，判断类型 T 是否为有效的调试策略
 * @tparam T 待检测的类型
 *
 * 要求 T 必须提供：
 * - 嵌套类型 cycle_counter_size_type（无符号整数）
 * - 静态方法 bkpt()
 * - 静态方法 enable_cycle_counter()
 * - 静态方法 disable_cycle_counter()
 * - 静态方法 get_cycle_counter()，且返回类型必须与 cycle_counter_size_type 相同
 * - 静态方法 is_cycle_counter_enabled() -> bool
 */
template <typename T>
struct is_valid_debug_policy : std::conjunction<has_bkpt_method<T>,
                                                has_enable_cycle_counter_method<T>,
                                                has_disable_cycle_counter_method<T>,
                                                has_get_cycle_counter_method<T>,
                                                is_correct_cycle_counter_return_type<T>,
                                                has_cycle_counter_size_type<T>,
                                                is_valid_cycle_counter_size_type<T>,
                                                has_is_cycle_counter_enabled_method<T>,
                                                is_correct_is_cycle_counter_enabled_return_type<T>> {};

template <typename T>
static constexpr bool is_valid_debug_policy_v = is_valid_debug_policy<T>::value;

// -----------------------------------------------------------------------------
// 可选功能检测
// -----------------------------------------------------------------------------

/**
 * @brief 检测静态方法 send_char(char)
 */
template <typename T, typename = void>
struct has_send_char_method : std::false_type {};
template <typename T>
struct has_send_char_method<T, std::void_t<decltype(T::send_char(std::declval<char>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_send_char_method_v = has_send_char_method<T>::value;

/**
 * @brief 检测策略是否包含嵌套类型 size_type
 */
template <typename T, typename = void>
struct has_size_type : std::false_type {};
template <typename T>
struct has_size_type<T, std::void_t<typename T::size_type>> : std::true_type {};
template <typename T>
static constexpr bool has_size_type_v = has_size_type<T>::value;

/**
 * @brief 检测 size_type 是否为无符号整数且大小不小于 std::size_t
 */
template <typename T>
struct is_valid_size_type : std::bool_constant<std::is_unsigned_v<typename T::size_type> &&
                                               (sizeof(typename T::size_type) >= sizeof(std::size_t))> {};
template <typename T>
static constexpr bool is_valid_size_type_v = is_valid_size_type<T>::value;

/**
 * @brief 检测静态方法 send_block(const std::byte*, size_type)
 */
template <typename T, typename = void>
struct has_send_block_method : std::false_type {};
template <typename T>
struct has_send_block_method<
    T,
    std::void_t<decltype(T::send_block(std::declval<const std::byte*>(), std::declval<typename T::size_type>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_send_block_method_v = has_send_block_method<T>::value;

/**
 * @brief 检测静态方法 is_ready()
 */
template <typename T, typename = void>
struct has_is_ready_method : std::false_type {};
template <typename T>
struct has_is_ready_method<T, std::void_t<decltype(T::is_ready())>> : std::true_type {};
template <typename T>
static constexpr bool has_is_ready_method_v = has_is_ready_method<T>::value;

/**
 * @brief 检测静态方法 is_ready() 的返回类型是否为 bool
 */
template <typename T, typename = void>
struct is_correct_is_ready_return_type : std::false_type {};
template <typename T>
struct is_correct_is_ready_return_type<T, std::void_t<decltype(T::is_ready())>>
    : std::is_same<decltype(T::is_ready()), bool> {};
template <typename T>
static constexpr bool is_correct_is_ready_return_type_v = is_correct_is_ready_return_type<T>::value;

} // namespace strat_os::hal::traits

namespace strat_os::hal
{

/**
 * @brief 调试适配器模板
 * @tparam DebugPolicy 具体的策略类，必须满足调试策略接口
 *
 * 该类将策略类包装为统一的静态接口，并进行编译期验证。
 * 所有方法均为内联且 noexcept，转发到策略类的对应静态方法。
 *
 * 可选功能（如 ITM 输出）通过 SFINAE 条件暴露，若策略未实现，
 * 则相应方法在适配器中不可用，从而避免代码膨胀。
 *
 * 使用示例：
 * @code
 * using MyDebug = Debug<CortexM3Debug>;
 * MyDebug::enable_cycle_counter();
 * auto cycles = MyDebug::get_cycle_counter();
 * MyDebug::send_char('A');
 * @endcode
 */
template <typename DebugPolicy, typename = std::enable_if_t<traits::is_valid_debug_policy_v<DebugPolicy>>>
struct Debug {
    /// 策略类型别名
    using Policy = DebugPolicy;

    // ----- 细粒度静态断言，提供清晰的错误信息 -----
    static_assert(traits::has_bkpt_method_v<Policy>, "Policy must provide bkpt()");
    static_assert(traits::has_enable_cycle_counter_method_v<Policy>, "Policy must provide enable_cycle_counter()");
    static_assert(traits::has_disable_cycle_counter_method_v<Policy>, "Policy must provide disable_cycle_counter()");
    static_assert(traits::has_get_cycle_counter_method_v<Policy>, "Policy must provide get_cycle_counter()");
    static_assert(traits::has_is_cycle_counter_enabled_method_v<Policy>,
                  "Policy must provide is_cycle_counter_enabled()");
    static_assert(traits::has_cycle_counter_size_type_v<Policy>,
                  "Policy must provide nested type 'cycle_counter_size_type'");
    static_assert(traits::is_valid_cycle_counter_size_type_v<Policy>,
                  "Policy::cycle_counter_size_type must be an unsigned integer type");
    static_assert((!traits::has_send_block_method_v<Policy> ||
                   (traits::has_size_type_v<Policy> && traits::is_valid_size_type_v<Policy>)),
                  "DebugPolicy defines send_block() but does not provide a valid 'size_type' (unsigned and at least "
                  "sizeof(size_t))");
    static_assert(traits::is_correct_cycle_counter_return_type_v<Policy>,
                  "Policy's get_cycle_counter() must return cycle_counter_size_type");
    static_assert(traits::is_correct_is_cycle_counter_enabled_return_type_v<Policy>,
                  "Policy's is_cycle_counter_enabled() must return bool");

    /// 周期计数器大小类型别名
    using cycle_counter_size_type = typename Policy::cycle_counter_size_type;

    // ----- 必需方法（所有平台必须实现）-----

    /**
     * @brief 插入断点（硬件断点指令）
     */
    inline static void bkpt() noexcept {
        Policy::bkpt();
    }

    /**
     * @brief 使能周期计数器
     */
    inline static void enable_cycle_counter() noexcept {
        Policy::enable_cycle_counter();
    }

    /**
     * @brief 禁用周期计数器
     */
    inline static void disable_cycle_counter() noexcept {
        Policy::disable_cycle_counter();
    }

    /**
     * @brief 获取当前周期计数值
     * @return 周期计数值（类型由策略定义）
     */
    [[nodiscard]] inline static cycle_counter_size_type get_cycle_counter() noexcept {
        return Policy::get_cycle_counter();
    }

    /**
     * @brief 查询周期计数器是否已使能
     * @return true 已使能，false 未使能
     */
    [[nodiscard]] inline static bool is_cycle_counter_enabled() noexcept {
        return Policy::is_cycle_counter_enabled();
    }

    // ----- 可选方法（ITM 软件跟踪）-----

    /**
     * @brief 发送一个字符（通过 ITM）
     * @param c 要发送的字符
     * @note 仅当策略提供 send_char() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_send_char_method_v<P>>>
    inline static void send_char(char c) noexcept {
        P::send_char(c);
    }

    /**
     * @brief 发送一个内存块（通过 ITM）
     * @param data 指向数据起始位置的指针
     * @param size 数据长度（字节）
     * @note 仅当策略提供 send_block() 且定义了有效的 size_type 时可用
     */
    template <typename P = Policy,
              typename   = std::enable_if_t<traits::has_size_type_v<P> && traits::is_valid_size_type_v<P> &&
                                            traits::has_send_block_method_v<P>>>
    inline static void send_block(const std::byte* data, typename P::size_type size) noexcept {
        P::send_block(data, size);
    }

    /**
     * @brief 便捷模板：发送任意类型对象（通过 ITM）
     * @param value 要发送的对象
     * @note 仅当策略支持 send_block() 且定义了有效的 size_type 时可用
     */
    template <typename T,
              typename P = Policy,
              typename   = std::enable_if_t<traits::has_send_block_method_v<P> && traits::has_size_type_v<P> &&
                                            traits::is_valid_size_type_v<P>>>
    static void send(const T& value) noexcept {
        send_block(reinterpret_cast<const std::byte*>(&value), static_cast<typename P::size_type>(sizeof(T)));
    }

    /**
     * @brief 查询 ITM 发送是否就绪
     * @return true 可发送，false 不可发送
     * @note 仅当策略提供 is_ready() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_is_ready_method_v<P>>>
    [[nodiscard]] inline static bool is_ready() noexcept {
        static_assert(traits::is_correct_is_ready_return_type_v<P>, "Policy's is_ready() must return bool");
        return P::is_ready();
    }
};

} // namespace strat_os::hal

#endif // STRATOS_HAL_DEBUG_HPP