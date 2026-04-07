/**
 * @file platform_context.hpp
 * @author StratOS Team
 * @brief 平台上下文策略接口与适配器
 * @version 1.0.0
 * @date 2026-04-07
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了硬件抽象层（HAL）的平台上下文策略接口，采用静态策略模式。
 * 平台上下文用于保存和恢复硬件相关的状态（如 FPU 寄存器、MPU 配置、安全扩展等），
 * 在任务切换时由调度器自动调用。
 *
 * 策略类必须定义以下内容：
 * - `supports_platform_context`：静态布尔常量，指示是否需要保存/恢复平台上下文
 * - `platform_context_type`：上下文数据结构（当 supports_platform_context == true 时必需）
 * - `save(platform_context_type*)`：静态方法，保存当前平台上下文到结构体
 * - `restore(const platform_context_type*)`：静态方法，从结构体恢复平台上下文
 *
 * 可选地，如果 supports_platform_context == false，则只需定义该常量。
 *
 * 该设计保证了零开销抽象，所有方法均为内联且 noexcept，适合嵌入式高安全环境。
 *
 * @note 平台上下文类型必须满足：标准布局、可平凡复制、可拷贝构造/赋值。
 * @warning 策略类必须保证所有方法永不抛出异常，适配器已标记 noexcept。
 */
#pragma once

#ifndef STRATOS_HAL_PLATFORM_CONTEXT_HPP
#define STRATOS_HAL_PLATFORM_CONTEXT_HPP

#include <type_traits>
#include <utility>

namespace strat_os::hal::traits
{

// ----------------------------------------------------------------------------
// 平台上下文类型检测
// ----------------------------------------------------------------------------

/**
 * @brief 检测类型 T 是否包含嵌套类型 platform_context_type
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct has_platform_context_type : std::false_type {};
template <typename T>
struct has_platform_context_type<T, std::void_t<typename T::platform_context_type>> : std::true_type {};
template <typename T>
static constexpr bool has_platform_context_type_v = has_platform_context_type<T>::value;

/**
 * @brief 检测 platform_context_type 是否为有效的平台上下文类型
 * @tparam T 待检测的策略类型（必须已定义 platform_context_type）
 * @details 要求类型满足：
 *          - 标准布局（standard layout）：与 C 兼容，偏移量可预测
 *          - 可平凡复制（trivially copyable）：可使用 memcpy 或内联汇编批量操作
 *          - 可拷贝构造（copy constructible）
 *          - 可拷贝赋值（copy assignable）
 */
template <typename T, typename = void>
struct is_valid_platform_context_type : std::false_type {};
template <typename T>
struct is_valid_platform_context_type<T, std::void_t<typename T::platform_context_type>>
    : std::conjunction<std::is_standard_layout<typename T::platform_context_type>,
                       std::is_trivially_copyable<typename T::platform_context_type>,
                       std::is_copy_constructible<typename T::platform_context_type>,
                       std::is_copy_assignable<typename T::platform_context_type>> {};
template <typename T>
static constexpr bool is_valid_platform_context_type_v = is_valid_platform_context_type<T>::value;

// ----------------------------------------------------------------------------
// supports_platform_context 常量检测
// ----------------------------------------------------------------------------

/**
 * @brief 检测类型 T 是否包含静态常量 supports_platform_context
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct has_supports_platform_context : std::false_type {};
template <typename T>
struct has_supports_platform_context<T, std::void_t<decltype(T::supports_platform_context)>> : std::true_type {};
template <typename T>
static constexpr bool has_supports_platform_context_v = has_supports_platform_context<T>::value;

/**
 * @brief 检测 supports_platform_context 是否为 const bool 类型
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct is_valid_supports_platform_context : std::false_type {};
template <typename T>
struct is_valid_supports_platform_context<T, std::void_t<decltype(T::supports_platform_context)>>
    : std::is_same<decltype(T::supports_platform_context), const bool> {};
template <typename T>
static constexpr bool is_valid_supports_platform_context_v = is_valid_supports_platform_context<T>::value;

// ----------------------------------------------------------------------------
// save / restore 方法检测
// ----------------------------------------------------------------------------

/**
 * @brief 检测静态方法 save(platform_context_type*)
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct has_save_method : std::false_type {};
template <typename T>
struct has_save_method<T, std::void_t<decltype(T::save(std::declval<typename T::platform_context_type*>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_save_method_v = has_save_method<T>::value;

/**
 * @brief 检测静态方法 restore(const platform_context_type*)
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct has_restore_method : std::false_type {};
template <typename T>
struct has_restore_method<T,
                          std::void_t<decltype(T::restore(std::declval<const typename T::platform_context_type*>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_restore_method_v = has_restore_method<T>::value;

// ----------------------------------------------------------------------------
// 组合检测：是否为有效的平台上下文策略
// ----------------------------------------------------------------------------

/**
 * @brief 组合检测，判断类型 T 是否为有效的平台上下文策略
 * @tparam T 待检测的策略类型
 *
 * 要求：
 * - 必须定义 supports_platform_context 常量（const bool）
 * - 如果 supports_platform_context == true，则必须同时提供：
 *   - platform_context_type（且满足标准布局、可平凡复制、可拷贝）
 *   - save(platform_context_type*) 方法
 *   - restore(const platform_context_type*) 方法
 * - 如果 supports_platform_context == false，则只需定义platform_context_type为空类
 */
template <typename T>
static constexpr bool is_valid_platform_context_policy_v =
    has_supports_platform_context_v<T> && is_valid_supports_platform_context_v<T> &&
    ((!T::supports_platform_context && std::is_empty_v<typename T::platform_context_type>) || (has_platform_context_type_v<T> && is_valid_platform_context_type_v<T> &&
                                       has_save_method_v<T> && has_restore_method_v<T>));

} // namespace strat_os::hal::traits

namespace strat_os::hal
{

/**
 * @brief 平台上下文适配器模板
 * @tparam PlatformContextPolicy 具体的策略类，必须满足平台上下文策略接口
 *
 * 该类将策略类包装为统一的静态接口，并进行编译期验证。
 * 所有方法均为内联且 noexcept，转发到策略类的对应静态方法。
 *
 * 若策略支持平台上下文（supports_platform_context == true），则提供 save/restore 方法；
 * 否则这些方法在适配器中不可用（通过 SFINAE 隐藏）。
 *
 * @par 使用示例
 * @code
 * // 假设 CortexM4PlatformPolicy 已定义
 * using MyPlatform = PlatformContext<CortexM4PlatformPolicy>;
 *
 * @endcode
 *
 * @note 策略类必须定义 supports_platform_context 常量，
 *       当为 true 时还必须定义 platform_context_type、save、restore。
 * @warning 平台上下文类型必须满足标准布局、可平凡复制、可拷贝等要求，
 *          否则编译期静态断言会失败。
 */
template <typename PlatformContextPolicy,
          typename = std::enable_if_t<traits::is_valid_platform_context_policy_v<PlatformContextPolicy>>>
struct PlatformContext {
    /// 策略类别名
    using Policy = PlatformContextPolicy;

    // ----- 细粒度静态断言，提供清晰的错误信息 -----
    static_assert(traits::has_supports_platform_context_v<Policy>,
                  "Policy must define static constant 'supports_platform_context'");
    static_assert(traits::is_valid_supports_platform_context_v<Policy>,
                  "Policy::supports_platform_context must be of type const bool");

    // 仅当 supports_platform_context 为 true 时才进行以下检查
    static_assert(!Policy::supports_platform_context || traits::has_platform_context_type_v<Policy>,
                  "Policy must define 'platform_context_type' when supports_platform_context is true");
    static_assert(
        !Policy::supports_platform_context || traits::is_valid_platform_context_type_v<Policy>,
        "platform_context_type must be standard layout, trivially copyable, copy constructible, and copy assignable");
    static_assert(!Policy::supports_platform_context || traits::has_save_method_v<Policy>,
                  "Policy must provide save(platform_context_type*) when supports_platform_context is true");
    static_assert(!Policy::supports_platform_context || traits::has_restore_method_v<Policy>,
                  "Policy must provide restore(const platform_context_type*) when supports_platform_context is true");

    /// 是否支持平台上下文（编译期常量）
    static constexpr bool supports_platform_context = Policy::supports_platform_context;

    /// 平台上下文数据类型（仅在 supports_platform_context 为 true 时有效）
    using platform_context_type = typename Policy::platform_context_type;

    // ----- 保存/恢复方法（仅在 supports_platform_context 为 true 时提供）-----

    /**
     * @brief 保存当前平台上下文到指定结构体
     * @param ctx 指向平台上下文结构体的指针
     * @note 仅当策略支持平台上下文时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<P::supports_platform_context>>
    inline static void save(platform_context_type* ctx) noexcept {
        P::save(ctx);
    }

    /**
     * @brief 从指定结构体恢复平台上下文
     * @param ctx 指向平台上下文结构体的指针（只读）
     * @note 仅当策略支持平台上下文时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<P::supports_platform_context>>
    inline static void restore(const platform_context_type* ctx) noexcept {
        P::restore(ctx);
    }
};

} // namespace strat_os::hal

#endif // STRATOS_HAL_PLATFORM_CONTEXT_HPP