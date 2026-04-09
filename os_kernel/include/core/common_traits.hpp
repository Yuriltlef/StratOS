/**
 * @file common_traits.hpp
 * @author StratOS Team
 * @brief 内核（Kernel）通用类型萃取
 * @version 1.0.0
 * @date 2026-04-01
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了 Kernel 各模块通用的类型萃取模板，用于编译期检测策略类
 * 是否满足接口要求。这些萃取可在不同的 Kernel 模块中复用，避免重复定义。
 *
 * 包含的萃取：
 * - 基本方法检测：init()
 *
 * 所有萃取均遵循静态策略模式的设计原则，通过 std::void_t 实现
 * 零开销的编译期检查。
 */
#pragma once

#ifndef STRATOS_KERNEL_COMMON_TRAITS_HPP
#define STRATOS_KERNEL_COMMON_TRAITS_HPP

#include <type_traits> // for std::false_type, std::true_type, std::void_t

namespace strat_os::kernel::traits
{
/**
 * @brief 检测类型 T 是否提供静态方法 init()
 * @tparam T 待检测的类型
 *
 * 该方法通常用于初始化内核数据结构或对象。
 */
template <typename T, typename = void>
struct has_init_method : std::false_type {};
template <typename T>
struct has_init_method<T, std::void_t<decltype(T::init())>> : std::true_type {};
template <typename T>
static constexpr bool has_init_method_v = has_init_method<T>::value;

} // namespace strat_os::kernel::traits

#endif // STRATOS_KERNEL_COMMON_TRAITS_HPP