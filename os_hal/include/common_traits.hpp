/**
 * @file common_traits.hpp
 * @author StratOS Team
 * @brief 硬件抽象层（HAL）通用类型萃取
 * @version 1.0.0
 * @date 2026-04-01
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了 HAL 各模块通用的类型萃取模板，用于编译期检测策略类
 * 是否满足接口要求。这些萃取可在不同的 HAL 模块（如中断控制器、
 * 系统控制等）中复用，避免重复定义。
 *
 * 包含的萃取：
 * - 优先级相关类型检测：priority_type、priority_group_type
 * - 基本方法检测：enable()、disable()
 *
 * 所有萃取均遵循静态策略模式的设计原则，通过 std::void_t 实现
 * 零开销的编译期检查。
 */
#pragma once

#ifndef STRATOS_HAL_COMMON_TRAITS_HPP
#define STRATOS_HAL_COMMON_TRAITS_HPP

#include <type_traits> // for std::false_type, std::true_type, std::void_t, std::is_unsigned

namespace strat_os::hal::traits
{

// ----------------------------------------------------------------------------
// 优先级类型检测
// ----------------------------------------------------------------------------

/**
 * @brief 检测类型 T 是否包含嵌套类型 priority_type
 * @tparam T 待检测的类型
 *
 * 该萃取用于检查策略类是否定义了优先级类型，例如：
 * @code
 * struct MyPolicy {
 *     using priority_type = uint8_t;
 * };
 * static_assert(traits::has_priority_type_v<MyPolicy>); // true
 * @endcode
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
 *
 * 该萃取用于确保优先级类型符合硬件要求（通常为无符号整数）。
 * 仅当 T 已定义 priority_type 时有效，否则结果为 false。
 *
 * @note 该萃取依赖于 has_priority_type，通常与 has_priority_type 组合使用。
 */
template <typename T, typename = void>
struct is_valid_priority_type : std::false_type {};
template <typename T>
struct is_valid_priority_type<T, std::void_t<typename T::priority_type>> : std::is_unsigned<typename T::priority_type> {
};
template <typename T>
inline constexpr bool is_valid_priority_type_v = is_valid_priority_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 priority_group_type
 * @tparam T 待检测的类型
 *
 * 优先级分组类型用于支持中断优先级分组（如 Cortex-M 的 PRIGROUP）。
 * 策略类可定义该类型以支持分组功能。
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
 *
 * 确保优先级分组类型为无符号整数，符合硬件要求。
 */
template <typename T, typename = void>
struct is_valid_priority_group_type : std::false_type {};
template <typename T>
struct is_valid_priority_group_type<T, std::void_t<typename T::priority_group_type>>
    : std::is_unsigned<typename T::priority_group_type> {};
template <typename T>
inline constexpr bool is_valid_priority_group_type_v = is_valid_priority_group_type<T>::value;

// ----------------------------------------------------------------------------
// 基本方法检测
// ----------------------------------------------------------------------------

/**
 * @brief 检测类型 T 是否提供静态方法 enable()
 * @tparam T 待检测的类型
 *
 * 该方法通常用于全局使能某个硬件模块（如中断控制器、MPU 等）。
 */
template <typename T, typename = void>
struct has_enable_method : std::false_type {};
template <typename T>
struct has_enable_method<T, std::void_t<decltype(T::enable())>> : std::true_type {};
template <typename T>
inline constexpr bool has_enable_method_v = has_enable_method<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 disable()
 * @tparam T 待检测的类型
 *
 * 该方法通常用于全局禁用某个硬件模块。
 */
template <typename T, typename = void>
struct has_disable_method : std::false_type {};
template <typename T>
struct has_disable_method<T, std::void_t<decltype(T::disable())>> : std::true_type {};
template <typename T>
inline constexpr bool has_disable_method_v = has_disable_method<T>::value;

} // namespace strat_os::hal::traits

#endif // STRATOS_HAL_COMMON_TRAITS_HPP