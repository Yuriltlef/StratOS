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

#include "os_kernel/include/core/tcb.hpp"
#include <cstddef>     // for std::size_t
#include <cstdint>     // for std::uintptr_t
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

/**
 * @brief 检测类型 T 是否包含静态常量 is_dynamic
 */
template <typename T, typename = void>
struct has_is_dynamic : std::false_type {};

template <typename T>
struct has_is_dynamic<T, std::void_t<decltype(T::is_dynamic)>> : std::true_type {};

template <typename T>
static constexpr bool has_is_dynamic_v = has_is_dynamic<T>::value;

/**
 * @brief 检测 is_dynamic 是否为 bool 类型
 */
template <typename T, typename = void>
struct is_valid_is_dynamic_type : std::false_type {};

template <typename T>
struct is_valid_is_dynamic_type<T, std::void_t<decltype(T::is_dynamic)>>
    : std::is_same<std::remove_cv_t<decltype(T::is_dynamic)>, bool> {};

template <typename T>
static constexpr bool is_valid_is_dynamic_type_v = is_valid_is_dynamic_type<T>::value;

/**
 * @brief 组合检测：判断类型 T 是否为有效的模式策略
 */
template <typename T>
struct is_valid_mode_policy : std::conjunction<has_is_dynamic<T>, is_valid_is_dynamic_type<T>> {};

template <typename T>
static constexpr bool is_valid_mode_policy_v = is_valid_mode_policy<T>::value;

/**
 * @brief 检测类型 T 是否包含静态常量 base
 */
template <typename T, typename = void>
struct has_layout_base : std::false_type {};

template <typename T>
struct has_layout_base<T, std::void_t<decltype(T::base)>> : std::true_type {};

template <typename T>
static constexpr bool has_layout_base_v = has_layout_base<T>::value;

/**
 * @brief 检测 base 的类型是否为 std::uintptr_t
 */
template <typename T, typename = void>
struct is_valid_layout_base_type : std::false_type {};

template <typename T>
struct is_valid_layout_base_type<T, std::void_t<decltype(T::base)>>
    : std::is_same<std::remove_cv_t<decltype(T::base)>, std::uintptr_t> {};

template <typename T>
static constexpr bool is_valid_layout_base_type_v = is_valid_layout_base_type<T>::value;

/**
 * @brief 检测类型 T 是否包含静态常量 size
 */
template <typename T, typename = void>
struct has_layout_size : std::false_type {};

template <typename T>
struct has_layout_size<T, std::void_t<decltype(T::size)>> : std::true_type {};

template <typename T>
static constexpr bool has_layout_size_v = has_layout_size<T>::value;

/**
 * @brief 检测 size 的类型是否为 std::size_t
 */
template <typename T, typename = void>
struct is_valid_layout_size_type : std::false_type {};

template <typename T>
struct is_valid_layout_size_type<T, std::void_t<decltype(T::size)>>
    : std::is_same<std::remove_cv_t<decltype(T::size)>, std::size_t> {};

template <typename T>
static constexpr bool is_valid_layout_size_type_v = is_valid_layout_size_type<T>::value;

/**
 * @brief 组合检测：判断类型 T 是否为有效的布局策略
 */
template <typename T>
struct is_valid_layout_policy : std::conjunction<has_layout_base<T>,
                                                 is_valid_layout_base_type<T>,
                                                 has_layout_size<T>,
                                                 is_valid_layout_size_type<T>> {};

template <typename T>
static constexpr bool is_valid_layout_policy_v = is_valid_layout_policy<T>::value;

/**
 * @brief 检测类型 T 是否具有 layout_policy 类型成员
 */
template <typename T, typename = void>
struct has_layout_policy : std::false_type {};

template <typename T>
struct has_layout_policy<T, std::void_t<typename T::layout_policy>> : std::true_type {};

template <typename T>
static constexpr bool has_layout_policy_v = has_layout_policy<T>::value;

/**
 * @brief 检测类型 T 是否具有 mode_policy 类型成员
 */
template <typename T, typename = void>
struct has_mode_policy : std::false_type {};

template <typename T>
struct has_mode_policy<T, std::void_t<typename T::mode_policy>> : std::true_type {};

template <typename T>
static constexpr bool has_mode_policy_v = has_mode_policy<T>::value;

/**
 * @brief 检测类型 T 是否具有 layout 类型成员
 */
template <typename T, typename = void>
struct has_layout_member : std::false_type {};

template <typename T>
struct has_layout_member<T, std::void_t<typename T::layout>> : std::true_type {};

template <typename T>
static constexpr bool has_layout_member_v = has_layout_member<T>::value;

/**
 * @brief 检测类型 T 是否具有 mode 类型成员
 */
template <typename T, typename = void>
struct has_mode_member : std::false_type {};

template <typename T>
struct has_mode_member<T, std::void_t<typename T::mode>> : std::true_type {};

template <typename T>
static constexpr bool has_mode_member_v = has_mode_member<T>::value;

/**
 * @brief 检测类型 T 是否为一个合法的布局类型（具有 base 和 size 常量）
 */
template <typename T, typename = void>
struct is_valid_layout_type : std::false_type {};

template <typename T>
struct is_valid_layout_type<T, std::void_t<decltype(T::base), decltype(T::size)>>
    : std::conjunction<std::is_same<std::remove_cv_t<decltype(T::base)>, std::uintptr_t>,
                       std::is_same<std::remove_cv_t<decltype(T::size)>, std::size_t>> {};

template <typename T>
static constexpr bool is_valid_layout_type_v = is_valid_layout_type<T>::value;

/**
 * @brief 检测类型 T 是否为一个合法的模式类型（具有 is_dynamic 常量且为 bool）
 */
template <typename T, typename = void>
struct is_valid_mode_type : std::false_type {};

template <typename T>
struct is_valid_mode_type<T, std::void_t<decltype(T::is_dynamic)>>
    : std::is_same<std::remove_cv_t<decltype(T::is_dynamic)>, bool> {};

template <typename T>
static constexpr bool is_valid_mode_type_v = is_valid_mode_type<T>::value;

/**
 * @brief 检测类型 T 是否为合法的 MemoryRegion
 *
 * 合法条件：
 * - 具有 layout_policy, mode_policy, layout, mode 四个嵌套类型
 * - layout 满足 is_valid_layout_type（即有 base 和 size）
 * - mode 满足 is_valid_mode_type（即有 is_dynamic 且为 bool）
 */
template <typename T>
struct is_region : std::conjunction<has_layout_policy<T>,
                                    has_mode_policy<T>,
                                    has_layout_member<T>,
                                    has_mode_member<T>,
                                    is_valid_layout_type<typename T::layout>,
                                    is_valid_mode_type<typename T::mode>> {};

template <typename T>
static constexpr bool is_region_v = is_region<T>::value;

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


} // namespace strat_os::kernel::traits

#endif // STRATOS_KERNEL_COMMON_TRAITS_HPP