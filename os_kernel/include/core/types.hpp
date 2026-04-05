/**
 * @file types.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-04-05
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifndef STRATOS_KERNEL_TYPES_HPP
#define STRATOS_KERNEL_TYPES_HPP

#include "os_kernel/config/kernel_config.hpp"
#include <type_traits> // for std::false_type, std::true_type

namespace strat_os::kernel::traits
{
/**
 * @brief 检测类型 T 是否包含嵌套类型 priority_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_priority_type : std::false_type {};
template <typename T>
struct has_priority_type<T, std::void_t<typename T::priority_type>> : std::true_type {};
template <typename T>
static constexpr bool has_reload_type_v = has_priority_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 tick_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_tick_type : std::false_type {};
template <typename T>
struct has_tick_type<T, std::void_t<typename T::tick_type>> : std::true_type {};
template <typename T>
static constexpr bool has_tick_type_v = has_tick_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 task_id_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_task_id_type : std::false_type {};
template <typename T>
struct has_task_id_type<T, std::void_t<typename T::task_id_type>> : std::true_type {};
template <typename T>
static constexpr bool has_task_id_type_v = has_task_id_type<T>::value;

/**
 * @brief 组合检测，判断类型 T 是否为有效的 KernelConfigPolicy 策略
 * @tparam T 待检测的类型
 *
 * 要求 T 必须定义 priority_type tick_type 和 task_id_type 类型
 */
template <typename T>
struct is_valid_kernel_config_policy : std::conjunction<has_priority_type<T>, has_tick_type<T>, has_task_id_type<T>> {};
template <typename T>
static constexpr bool is_valid_kernel_config_policy_v = is_valid_kernel_config_policy<T>::value;

} // namespace strat_os::kernel::traits

namespace strat_os::kernel
{

template <typename KernelConfigPolicy = config::DefaultKernelConfigPolicy,
          typename                    = std::enable_if_t<traits::is_valid_kernel_config_policy_v<KernelConfigPolicy>>>
struct KernelTypes {
    using Policy        = KernelConfigPolicy;
    using priority_type = typename Policy::priority_type;
    using tick_type     = typename Policy::tick_type;
    using task_id_type  = typename Policy::task_id_type;
};

} // namespace strat_os::kernel

#endif // STRATOS_KERNEL_TYPES_HPP