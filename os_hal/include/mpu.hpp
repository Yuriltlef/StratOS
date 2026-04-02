/**
 * @file mpu.hpp
 * @author StratOS Team
 * @brief 内存保护单元（MPU）策略接口与适配器
 * @version 1.0.0
 * @date 2026-04-01
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了硬件抽象层（HAL）的内存保护单元（MPU）抽象接口，采用静态策略模式。
 * MPU 用于实现内存区域访问权限控制、任务隔离、栈溢出保护等安全功能。
 *
 * 主要组件：
 * - traits 命名空间：提供类型萃取，用于验证策略类是否符合接口要求，
 *   包括必需嵌套类型、静态方法及可选增强方法的检测。
 * - Mpu 适配器模板：接收一个策略类，静态断言其合法性，并转发所有 MPU 操作。
 *
 * 该设计保证了零开销抽象，所有函数均为内联且 noexcept，适合嵌入式高安全环境。
 *
 * 策略类必须定义以下嵌套类型：
 * - region_index_type：区域索引类型（通常为 uint8_t 或 uint32_t）
 * - region_address_type：区域基址类型（通常为 uintptr_t）
 * - region_size_type：区域大小类型（通常为 uint32_t）
 * - region_attributes_type：区域属性类型（平台相关的位掩码）
 * - region_info_type：包含完整区域信息的结构体，必须包含 base、size、attr、enabled 成员
 *
 * 必需静态方法：
 * - enable() / disable() / is_enabled()
 * - set_region(region_index_type, region_address_type, region_size_type, region_attributes_type, bool)
 * - disable_region(region_index_type)
 * - is_mpu_fault() / get_fault_address() / clear_fault()
 * - region_count()
 * - get_region(region_index_type) -> region_info_type
 * - is_readable(region_address_type) / is_writable(region_address_type) / is_executable(region_address_type)
 *
 * 可选增强方法（通过 SFINAE 自动暴露）：
 * - get_fault_access_type() 获取故障访问类型
 * - set_background_region() / is_background_region_enabled() 配置背景区域
 * - disable_subregion() / get_subregion_mask() 子区域控制
 * - is_overlap() 区域重叠检测
 * - region_priority_by_number() 区域优先级顺序常量
 *
 * @note 所有方法均假设在特权模式下调用，且调用前已确保系统状态合法。
 * @warning 策略类必须保证所有方法永不抛出异常，适配器已标记 noexcept。
 */
#pragma once

#ifndef STRATOS_HAL_MPU_HPP
#define STRATOS_HAL_MPU_HPP

#include <type_traits> // for std::false_type, std::true_type, etc.
#include <utility>     // for std::declval

namespace strat_os::hal::traits
{

// ----------------------------------------------------------------------------
// 基础类型检测
// ----------------------------------------------------------------------------

/**
 * @brief 检测类型 T 是否包含嵌套类型 region_index_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_region_index_type : std::false_type {};
template <typename T>
struct has_region_index_type<T, std::void_t<typename T::region_index_type>> : std::true_type {};
template <typename T>
static constexpr bool has_region_index_type_v = has_region_index_type<T>::value;

/**
 * @brief 检测类型 T 嵌套类型 region_index_type 是否为无符号整数类型
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct is_valid_region_index_type : std::false_type {};
template <typename T>
struct is_valid_region_index_type<T, std::void_t<typename T::region_index_type>>
    : std::is_unsigned<typename T::region_index_type> {};
template <typename T>
static constexpr bool is_valid_region_index_type_v = is_valid_region_index_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 region_address_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_region_address_type : std::false_type {};
template <typename T>
struct has_region_address_type<T, std::void_t<typename T::region_address_type>> : std::true_type {};
template <typename T>
static constexpr bool has_region_address_type_v = has_region_address_type<T>::value;

/**
 * @brief 检测类型 T 嵌套类型 region_address_type 是否为无符号整数类型
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct is_valid_region_address_type : std::false_type {};
template <typename T>
struct is_valid_region_address_type<T, std::void_t<typename T::region_address_type>>
    : std::is_unsigned<typename T::region_address_type> {};
template <typename T>
static constexpr bool is_valid_region_address_type_v = is_valid_region_address_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 region_size_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_region_size_type : std::false_type {};
template <typename T>
struct has_region_size_type<T, std::void_t<typename T::region_size_type>> : std::true_type {};
template <typename T>
static constexpr bool has_region_size_type_v = has_region_size_type<T>::value;

/**
 * @brief 检测类型 T 嵌套类型 region_size_type 是否为无符号整数类型
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct is_valid_region_size_type : std::false_type {};
template <typename T>
struct is_valid_region_size_type<T, std::void_t<typename T::region_size_type>>
    : std::is_unsigned<typename T::region_size_type> {};
template <typename T>
static constexpr bool is_valid_region_size_type_v = is_valid_region_size_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 region_attributes_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_region_attributes_type : std::false_type {};
template <typename T>
struct has_region_attributes_type<T, std::void_t<typename T::region_attributes_type>> : std::true_type {};
template <typename T>
static constexpr bool has_region_attributes_type_v = has_region_attributes_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 region_info_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_region_info_type : std::false_type {};
template <typename T>
struct has_region_info_type<T, std::void_t<typename T::region_info_type>> : std::true_type {};
template <typename T>
static constexpr bool has_region_info_type_v = has_region_info_type<T>::value;

// ----------------------------------------------------------------------------
// region_info 成员检测
// ----------------------------------------------------------------------------

/**
 * @brief 检测 region_info_type 是否包含成员 base
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct has_region_info_base : std::false_type {};
template <typename T>
struct has_region_info_base<T, std::void_t<decltype(std::declval<typename T::region_info_type>().base)>>
    : std::true_type {};
template <typename T>
static constexpr bool has_region_info_base_v = has_region_info_base<T>::value;

/**
 * @brief 检测 region_info_type 是否包含成员 size
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct has_region_info_size : std::false_type {};
template <typename T>
struct has_region_info_size<T, std::void_t<decltype(std::declval<typename T::region_info_type>().size)>>
    : std::true_type {};
template <typename T>
static constexpr bool has_region_info_size_v = has_region_info_size<T>::value;

/**
 * @brief 检测 region_info_type 是否包含成员 attr
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct has_region_info_attr : std::false_type {};
template <typename T>
struct has_region_info_attr<T, std::void_t<decltype(std::declval<typename T::region_info_type>().attr)>>
    : std::true_type {};
template <typename T>
static constexpr bool has_region_info_attr_v = has_region_info_attr<T>::value;

/**
 * @brief 检测 region_info_type 是否包含成员 enabled
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct has_region_info_enabled : std::false_type {};
template <typename T>
struct has_region_info_enabled<T, std::void_t<decltype(std::declval<typename T::region_info_type>().enabled)>>
    : std::true_type {};
template <typename T>
static constexpr bool has_region_info_enabled_v = has_region_info_enabled<T>::value;

/**
 * @brief 检测 region_info_type 成员 enabled 是否为 bool 类型
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct is_valid_region_info_enabled_type : std::false_type {};
template <typename T>
struct is_valid_region_info_enabled_type<T, std::void_t<decltype(std::declval<typename T::region_info_type>().enabled)>>
    : std::is_same<decltype(std::declval<typename T::region_info_type>().enabled), bool> {};
template <typename T>
static constexpr bool is_valid_region_info_enabled_type_v = is_valid_region_info_enabled_type<T>::value;

// ----------------------------------------------------------------------------
// 必需方法检测
// ----------------------------------------------------------------------------

/**
 * @brief 检测静态方法 enable()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_enable_method : std::false_type {};
template <typename T>
struct has_mpu_enable_method<T, std::void_t<decltype(T::enable())>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_enable_method_v = has_mpu_enable_method<T>::value;

/**
 * @brief 检测静态方法 disable()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_disable_method : std::false_type {};
template <typename T>
struct has_mpu_disable_method<T, std::void_t<decltype(T::disable())>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_disable_method_v = has_mpu_disable_method<T>::value;

/**
 * @brief 检测静态方法 is_enabled()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_is_enabled_method : std::false_type {};
template <typename T>
struct has_mpu_is_enabled_method<T, std::void_t<decltype(T::is_enabled())>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_is_enabled_method_v = has_mpu_is_enabled_method<T>::value;

/**
 * @brief 检测静态方法 set_region()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_set_region_method : std::false_type {};
template <typename T>
struct has_mpu_set_region_method<T,
                                 std::void_t<decltype(T::set_region(std::declval<typename T::region_index_type>(),
                                                                    std::declval<typename T::region_address_type>(),
                                                                    std::declval<typename T::region_size_type>(),
                                                                    std::declval<typename T::region_attributes_type>(),
                                                                    std::declval<bool>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_set_region_method_v = has_mpu_set_region_method<T>::value;

/**
 * @brief 检测静态方法 disable_region()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_disable_region_method : std::false_type {};
template <typename T>
struct has_mpu_disable_region_method<
    T,
    std::void_t<decltype(T::disable_region(std::declval<typename T::region_index_type>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_disable_region_method_v = has_mpu_disable_region_method<T>::value;

/**
 * @brief 检测静态方法 is_mpu_fault()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_is_mpu_fault_method : std::false_type {};
template <typename T>
struct has_mpu_is_mpu_fault_method<T, std::void_t<decltype(T::is_mpu_fault())>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_is_mpu_fault_method_v = has_mpu_is_mpu_fault_method<T>::value;

/**
 * @brief 检测静态方法 get_fault_address()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_get_fault_address_method : std::false_type {};
template <typename T>
struct has_mpu_get_fault_address_method<T, std::void_t<decltype(T::get_fault_address())>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_get_fault_address_method_v = has_mpu_get_fault_address_method<T>::value;

/**
 * @brief 检测静态方法 clear_fault()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_clear_fault_method : std::false_type {};
template <typename T>
struct has_mpu_clear_fault_method<T, std::void_t<decltype(T::clear_fault())>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_clear_fault_method_v = has_mpu_clear_fault_method<T>::value;

/**
 * @brief 检测静态方法 region_count()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_region_count_method : std::false_type {};
template <typename T>
struct has_mpu_region_count_method<T, std::void_t<decltype(T::region_count())>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_region_count_method_v = has_mpu_region_count_method<T>::value;

/**
 * @brief 检测静态方法 get_region()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_get_region_method : std::false_type {};
template <typename T>
struct has_mpu_get_region_method<T, std::void_t<decltype(T::get_region(std::declval<typename T::region_index_type>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_mpu_get_region_method_v = has_mpu_get_region_method<T>::value;

/**
 * @brief 检测静态方法 is_readable()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_is_readable_method : std::false_type {};
template <typename T>
struct has_mpu_is_readable_method<
    T,
    std::void_t<decltype(T::is_readable(std::declval<typename T::region_address_type>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_is_readable_method_v = has_mpu_is_readable_method<T>::value;

/**
 * @brief 检测静态方法 is_writable()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_is_writable_method : std::false_type {};
template <typename T>
struct has_mpu_is_writable_method<
    T,
    std::void_t<decltype(T::is_writable(std::declval<typename T::region_address_type>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_is_writable_method_v = has_mpu_is_writable_method<T>::value;

/**
 * @brief 检测静态方法 is_executable()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_is_executable_method : std::false_type {};
template <typename T>
struct has_mpu_is_executable_method<
    T,
    std::void_t<decltype(T::is_executable(std::declval<typename T::region_address_type>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_is_executable_method_v = has_mpu_is_executable_method<T>::value;

// ----------------------------------------------------------------------------
// 可选方法检测
// ----------------------------------------------------------------------------

/**
 * @brief 检测静态方法 get_fault_access_type()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_get_fault_access_type_method : std::false_type {};
template <typename T>
struct has_mpu_get_fault_access_type_method<T, std::void_t<decltype(T::get_fault_access_type())>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_get_fault_access_type_method_v = has_mpu_get_fault_access_type_method<T>::value;

/**
 * @brief 检测静态方法 set_background_region()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_set_background_region_method : std::false_type {};
template <typename T>
struct has_mpu_set_background_region_method<
    T,
    std::void_t<decltype(T::set_background_region(std::declval<typename T::region_attributes_type>(),
                                                  std::declval<bool>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_set_background_region_method_v = has_mpu_set_background_region_method<T>::value;

/**
 * @brief 检测静态方法 is_background_region_enabled()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_is_background_region_enabled_method : std::false_type {};
template <typename T>
struct has_mpu_is_background_region_enabled_method<T, std::void_t<decltype(T::is_background_region_enabled())>>
    : std::true_type {};
template <typename T>
static constexpr bool has_mpu_is_background_region_enabled_method_v =
    has_mpu_is_background_region_enabled_method<T>::value;

/**
 * @brief 检测静态方法 disable_subregion()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_disable_subregion_method : std::false_type {};
template <typename T>
struct has_mpu_disable_subregion_method<
    T,
    std::void_t<decltype(T::disable_subregion(std::declval<typename T::region_index_type>(),
                                              std::declval<typename T::region_index_type>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_disable_subregion_method_v = has_mpu_disable_subregion_method<T>::value;

/**
 * @brief 检测静态方法 get_subregion_mask()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_get_subregion_mask_method : std::false_type {};
template <typename T>
struct has_mpu_get_subregion_mask_method<
    T,
    std::void_t<decltype(T::get_subregion_mask(std::declval<typename T::region_index_type>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_mpu_get_subregion_mask_method_v = has_mpu_get_subregion_mask_method<T>::value;

/**
 * @brief 检测静态方法 is_overlap()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_is_overlap_method : std::false_type {};
template <typename T>
struct has_mpu_is_overlap_method<T,
                                 std::void_t<decltype(T::is_overlap(std::declval<typename T::region_index_type>(),
                                                                    std::declval<typename T::region_index_type>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_mpu_is_overlap_method_v = has_mpu_is_overlap_method<T>::value;

/**
 * @brief 检测静态方法 region_priority_by_number()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_mpu_region_priority_by_number_method : std::false_type {};
template <typename T>
struct has_mpu_region_priority_by_number_method<T, std::void_t<decltype(T::region_priority_by_number())>>
    : std::true_type {};
template <typename T>
static constexpr bool has_mpu_region_priority_by_number_method_v = has_mpu_region_priority_by_number_method<T>::value;

// ----------------------------------------------------------------------------
// 返回类型检查
// ----------------------------------------------------------------------------

/**
 * @brief 检测 is_enabled() 的返回类型是否为 bool
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_correct_mpu_is_enabled_return : std::is_same<decltype(T::is_enabled()), bool> {};
template <typename T>
static constexpr bool is_correct_mpu_is_enabled_return_v = is_correct_mpu_is_enabled_return<T>::value;

/**
 * @brief 检测 is_mpu_fault() 的返回类型是否为 bool
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_correct_mpu_is_mpu_fault_return : std::is_same<decltype(T::is_mpu_fault()), bool> {};
template <typename T>
static constexpr bool is_correct_mpu_is_mpu_fault_return_v = is_correct_mpu_is_mpu_fault_return<T>::value;

/**
 * @brief 检测 get_fault_address() 的返回类型是否为 region_address_type
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_correct_mpu_get_fault_address_return
    : std::is_same<decltype(T::get_fault_address()), typename T::region_address_type> {};
template <typename T>
static constexpr bool is_correct_mpu_get_fault_address_return_v = is_correct_mpu_get_fault_address_return<T>::value;

/**
 * @brief 检测 region_count() 的返回类型是否为 region_index_type
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_correct_mpu_region_count_return : std::is_same<decltype(T::region_count()), typename T::region_index_type> {};
template <typename T>
static constexpr bool is_correct_mpu_region_count_return_v = is_correct_mpu_region_count_return<T>::value;

/**
 * @brief 检测 get_region() 的返回类型是否为 region_info_type
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_correct_mpu_get_region_return
    : std::is_same<decltype(T::get_region(std::declval<typename T::region_index_type>())),
                   typename T::region_info_type> {};
template <typename T>
static constexpr bool is_correct_mpu_get_region_return_v = is_correct_mpu_get_region_return<T>::value;

/**
 * @brief 检测 is_readable() 的返回类型是否为 bool
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_correct_mpu_is_readable_return
    : std::is_same<decltype(T::is_readable(std::declval<typename T::region_address_type>())), bool> {};
template <typename T>
static constexpr bool is_correct_mpu_is_readable_return_v = is_correct_mpu_is_readable_return<T>::value;

/**
 * @brief 检测 is_writable() 的返回类型是否为 bool
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_correct_mpu_is_writable_return
    : std::is_same<decltype(T::is_writable(std::declval<typename T::region_address_type>())), bool> {};
template <typename T>
static constexpr bool is_correct_mpu_is_writable_return_v = is_correct_mpu_is_writable_return<T>::value;

/**
 * @brief 检测 is_executable() 的返回类型是否为 bool
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_correct_mpu_is_executable_return
    : std::is_same<decltype(T::is_executable(std::declval<typename T::region_address_type>())), bool> {};
template <typename T>
static constexpr bool is_correct_mpu_is_executable_return_v = is_correct_mpu_is_executable_return<T>::value;

/**
 * @brief 检测 is_background_region_enabled() 的返回类型是否为 bool
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_correct_is_background_region_enabled_return
    : std::is_same<decltype(T::is_background_region_enabled()), bool> {};
template <typename T>
static constexpr bool is_correct_is_background_region_enabled_return_v =
    is_correct_is_background_region_enabled_return<T>::value;

/**
 * @brief 检测 is_overlap() 的返回类型是否为 bool
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_correct_is_overlap_return
    : std::is_same<decltype(T::is_overlap(std::declval<typename T::region_index_type>(),
                                          std::declval<typename T::region_index_type>())),
                   bool> {};
template <typename T>
static constexpr bool is_correct_is_overlap_return_v = is_correct_is_overlap_return<T>::value;

// ----------------------------------------------------------------------------
// 组合检测
// ----------------------------------------------------------------------------

/**
 * @brief 组合检测：判断类型 T 是否为有效的 MPU 策略
 * @tparam T 待检测的类型
 *
 * 要求 T 必须提供所有必需嵌套类型、成员及静态方法，且返回值类型正确。
 * 可选增强方法不在此检测范围内。
 */
template <typename T>
struct is_valid_mpu_policy : std::conjunction<has_region_index_type<T>,
                                              has_region_address_type<T>,
                                              has_region_size_type<T>,
                                              is_valid_region_size_type<T>,
                                              is_valid_region_address_type<T>,
                                              is_valid_region_index_type<T>,
                                              has_region_attributes_type<T>,
                                              has_region_info_type<T>,
                                              has_region_info_base<T>,
                                              has_region_info_size<T>,
                                              has_region_info_attr<T>,
                                              has_region_info_enabled<T>,
                                              is_valid_region_info_enabled_type<T>,
                                              has_mpu_enable_method<T>,
                                              has_mpu_disable_method<T>,
                                              has_mpu_is_enabled_method<T>,
                                              is_correct_mpu_is_enabled_return<T>,
                                              has_mpu_set_region_method<T>,
                                              has_mpu_disable_region_method<T>,
                                              has_mpu_is_mpu_fault_method<T>,
                                              is_correct_mpu_is_mpu_fault_return<T>,
                                              has_mpu_get_fault_address_method<T>,
                                              is_correct_mpu_get_fault_address_return<T>,
                                              has_mpu_clear_fault_method<T>,
                                              has_mpu_region_count_method<T>,
                                              is_correct_mpu_region_count_return<T>,
                                              has_mpu_get_region_method<T>,
                                              is_correct_mpu_get_region_return<T>,
                                              has_mpu_is_readable_method<T>,
                                              is_correct_mpu_is_readable_return<T>,
                                              has_mpu_is_writable_method<T>,
                                              is_correct_mpu_is_writable_return<T>,
                                              has_mpu_is_executable_method<T>,
                                              is_correct_mpu_is_executable_return<T>> {};

template <typename T>
static constexpr bool is_valid_mpu_policy_v = is_valid_mpu_policy<T>::value;
} // namespace strat_os::hal::traits

namespace strat_os::hal
{

/**
 * @brief MPU 适配器模板
 * @tparam MpuPolicy 具体的策略类，必须满足 MPU 策略接口
 *
 * 该类将策略类包装为统一的静态接口，并进行编译期验证。
 * 所有方法均为内联且 noexcept，转发到策略类的对应静态方法。
 *
 * 若策略支持可选增强方法，适配器会通过 SFINAE 自动暴露相应重载。
 *
 * @par 使用示例
 * @code
 * // 假设 CortexM3MPUPolicy 已实现所有必需方法
 * using MyMPU = Mpu<CortexM3MPUPolicy>;
 *
 * // 全局使能 MPU
 * MyMPU::enable();
 *
 * // 配置区域 0：基址 0x20000000，大小 1KB，读写权限，使能
 * MyMPU::set_region(0, 0x20000000, 1024, MyMPU::READ_WRITE, true);
 *
 * // 检查地址是否可读
 * if (MyMPU::is_readable(0x20000000)) { ... }
 *
 * // 故障处理
 * if (MyMPU::is_mpu_fault()) {
 *     auto addr = MyMPU::get_fault_address();
 *     MyMPU::clear_fault();
 * }
 * @endcode
 *
 * @note 策略类必须定义 region_index_type、region_address_type、region_size_type、
 *       region_attributes_type 和 region_info_type（包含 base、size、attr、enabled 成员）。
 * @warning 所有方法均标记 noexcept，策略类实现也必须保证不抛出异常。
 */
template <typename MpuPolicy, typename = std::enable_if_t<traits::is_valid_mpu_policy_v<MpuPolicy>>>
struct Mpu {
    /// 策略类别名
    using Policy = MpuPolicy;

    // ----- 细粒度静态断言，提供清晰的错误信息 -----
    static_assert(traits::has_region_index_type_v<Policy>, "Policy must define region_index_type");
    static_assert(traits::has_region_address_type_v<Policy>, "Policy must define region_address_type");
    static_assert(traits::has_region_size_type_v<Policy>, "Policy must define region_size_type");
    static_assert(traits::has_region_attributes_type_v<Policy>, "Policy must define region_attributes_type");
    static_assert(traits::has_region_info_type_v<Policy>, "Policy must define region_info_type");
    static_assert(traits::has_region_info_base_v<Policy>, "region_info_type must have member 'base'");
    static_assert(traits::has_region_info_size_v<Policy>, "region_info_type must have member 'size'");
    static_assert(traits::has_region_info_attr_v<Policy>, "region_info_type must have member 'attr'");
    static_assert(traits::has_region_info_enabled_v<Policy>, "region_info_type must have member 'enabled'");
    static_assert(traits::has_mpu_enable_method_v<Policy>, "Policy must provide static method enable()");
    static_assert(traits::has_mpu_disable_method_v<Policy>, "Policy must provide static method disable()");
    static_assert(traits::has_mpu_is_enabled_method_v<Policy>, "Policy must provide static method is_enabled()");
    static_assert(traits::is_correct_mpu_is_enabled_return_v<Policy>, "is_enabled() must return bool");
    static_assert(traits::has_mpu_set_region_method_v<Policy>, "Policy must provide static method set_region(...)");
    static_assert(traits::has_mpu_disable_region_method_v<Policy>,
                  "Policy must provide static method disable_region(region_index_type)");
    static_assert(traits::has_mpu_is_mpu_fault_method_v<Policy>, "Policy must provide static method is_mpu_fault()");
    static_assert(traits::is_correct_mpu_is_mpu_fault_return_v<Policy>, "is_mpu_fault() must return bool");
    static_assert(traits::has_mpu_get_fault_address_method_v<Policy>,
                  "Policy must provide static method get_fault_address()");
    static_assert(traits::is_correct_mpu_get_fault_address_return_v<Policy>,
                  "get_fault_address() must return region_address_type");
    static_assert(traits::has_mpu_clear_fault_method_v<Policy>, "Policy must provide static method clear_fault()");
    static_assert(traits::has_mpu_region_count_method_v<Policy>, "Policy must provide static method region_count()");
    static_assert(traits::is_correct_mpu_region_count_return_v<Policy>, "region_count() must return region_index_type");
    static_assert(traits::has_mpu_get_region_method_v<Policy>,
                  "Policy must provide static method get_region(region_index_type)");
    static_assert(traits::is_correct_mpu_get_region_return_v<Policy>,
                  "get_region(region_index_type) must return region_info_type");
    static_assert(traits::has_mpu_is_readable_method_v<Policy>,
                  "Policy must provide static method is_readable(region_address_type)");
    static_assert(traits::is_correct_mpu_is_readable_return_v<Policy>,
                  "is_readable(region_address_type) must return bool");
    static_assert(traits::has_mpu_is_writable_method_v<Policy>,
                  "Policy must provide static method is_writable(region_address_type)");
    static_assert(traits::is_correct_mpu_is_writable_return_v<Policy>,
                  "is_writable(region_address_type) must return bool");
    static_assert(traits::has_mpu_is_executable_method_v<Policy>,
                  "Policy must provide static method is_executable(region_address_type)");
    static_assert(traits::is_correct_mpu_is_executable_return_v<Policy>,
                  "is_executable(region_address_type) must return bool");
    static_assert(traits::is_valid_region_address_type_v<Policy>,
                  "region_address_type must be an unsigned integer type");
    static_assert(traits::is_valid_region_size_type_v<Policy>, "region_size_type must be an unsigned integer type");
    static_assert(traits::is_valid_region_index_type_v<Policy>, "region_index_type must be an unsigned integer type");
    static_assert(traits::is_valid_region_info_enabled_type_v<Policy>,
                  "region_info_type::enabled must be of type bool");

    /// 区域索引类型（取自策略类）
    using region_index_type = typename Policy::region_index_type;
    /// 区域基址类型（取自策略类）
    using region_address_type = typename Policy::region_address_type;
    /// 区域大小类型（取自策略类）
    using region_size_type = typename Policy::region_size_type;
    /// 区域属性类型（取自策略类）
    using region_attributes_type = typename Policy::region_attributes_type;
    /// 区域信息结构体类型（取自策略类）
    using region_info_type = typename Policy::region_info_type;

    // ----- 必需操作 -----

    /**
     * @brief 全局使能 MPU
     */
    inline static void enable() noexcept {
        Policy::enable();
    }

    /**
     * @brief 全局禁用 MPU
     */
    inline static void disable() noexcept {
        Policy::disable();
    }

    /**
     * @brief 查询 MPU 是否已使能
     * @return true 如果 MPU 已使能，否则 false
     */
    inline static bool is_enabled() noexcept {
        return Policy::is_enabled();
    }

    /**
     * @brief 配置 MPU 区域
     * @param idx     区域索引（0 ~ region_count()-1）
     * @param base    区域基址（必须按平台要求对齐）
     * @param size    区域大小（必须为平台支持的尺寸）
     * @param attr    区域属性（由策略类定义的位掩码）
     * @param enable  是否使能该区域
     */
    inline static void set_region(region_index_type idx,
                                  region_address_type base,
                                  region_size_type size,
                                  region_attributes_type attr,
                                  bool enable) noexcept {
        Policy::set_region(idx, base, size, attr, enable);
    }

    /**
     * @brief 禁用指定区域（不清除配置，仅清除使能位）
     * @param idx 区域索引
     */
    inline static void disable_region(region_index_type idx) noexcept {
        Policy::disable_region(idx);
    }

    /**
     * @brief 查询是否发生了 MPU 访问违规（MemManage 故障）
     * @return true 如果当前有未处理的 MPU 故障
     */
    [[nodiscard]] inline static bool is_mpu_fault() noexcept {
        return Policy::is_mpu_fault();
    }

    /**
     * @brief 获取导致 MPU 故障的内存地址
     * @return 故障地址
     */
    [[nodiscard]] inline static region_address_type get_fault_address() noexcept {
        return Policy::get_fault_address();
    }

    /**
     * @brief 清除 MPU 故障标志
     */
    inline static void clear_fault() noexcept {
        Policy::clear_fault();
    }

    /**
     * @brief 返回 MPU 支持的最大区域数
     * @return 区域数量（编译期常量）
     */
    [[nodiscard]] inline static constexpr region_index_type region_count() noexcept {
        return Policy::region_count();
    }

    /**
     * @brief 获取指定区域的完整配置信息
     * @param idx 区域索引
     * @return 包含 base、size、attr、enabled 的结构体
     * @note 主要用于调试和诊断，低频率调用
     */
    [[nodiscard]] inline static region_info_type get_region(region_index_type idx) noexcept {
        return Policy::get_region(idx);
    }

    /**
     * @brief 检查给定地址是否可读
     * @param addr 内存地址
     * @return true 如果可读，否则 false
     * @note 用于系统调用参数合法性检查
     */
    [[nodiscard]] inline static bool is_readable(region_address_type addr) noexcept {
        return Policy::is_readable(addr);
    }

    /**
     * @brief 检查给定地址是否可写
     * @param addr 内存地址
     * @return true 如果可写，否则 false
     * @note 用于系统调用参数合法性检查
     */
    [[nodiscard]] inline static bool is_writable(region_address_type addr) noexcept {
        return Policy::is_writable(addr);
    }

    /**
     * @brief 检查给定地址是否可执行
     * @param addr 内存地址
     * @return true 如果可执行，否则 false
     */
    [[nodiscard]] inline static bool is_executable(region_address_type addr) noexcept {
        return Policy::is_executable(addr);
    }

    // ----- 可选增强功能（通过 SFINAE 条件暴露）-----

    /**
     * @brief 获取导致 MPU 故障的访问类型
     * @return 访问类型（由策略类定义，通常为枚举）
     * @note 仅当策略提供 get_fault_access_type() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_mpu_get_fault_access_type_method_v<P>>>
    [[nodiscard]] inline static auto get_fault_access_type() noexcept {
        return P::get_fault_access_type();
    }

    /**
     * @brief 配置背景区域（默认内存映射）
     * @param attr   背景区域的属性（由策略类定义）
     * @param enable 是否使能背景区域（若禁用，未匹配区域将触发故障）
     * @note 仅当策略提供 set_background_region() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_mpu_set_background_region_method_v<P>>>
    inline static void set_background_region(typename P::region_attributes_type attr, bool enable) noexcept {
        P::set_background_region(attr, enable);
    }

    /**
     * @brief 查询背景区域是否使能
     * @return true 如果背景区域已使能
     * @note 仅当策略提供 is_background_region_enabled() 时可用
     */
    template <typename P = Policy,
              typename   = std::enable_if_t<traits::has_mpu_is_background_region_enabled_method_v<P>>>
    [[nodiscard]] inline static bool is_background_region_enabled() noexcept {
        static_assert(traits::is_correct_is_background_region_enabled_return_v<P>,
                      "is_background_region_enabled() must return bool");
        return P::is_background_region_enabled();
    }

    /**
     * @brief 禁用指定区域的某个子区域（仅对支持子区域的 MPU 有效）
     * @param region_idx  区域索引
     * @param subregion_idx 子区域索引（0-7）
     * @note 仅当策略提供 disable_subregion() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_mpu_disable_subregion_method_v<P>>>
    inline static void disable_subregion(typename P::region_index_type region_idx,
                                         typename P::region_index_type subregion_idx) noexcept {
        P::disable_subregion(region_idx, subregion_idx);
    }

    /**
     * @brief 获取指定区域的子区域禁用掩码
     * @param region_idx 区域索引
     * @return 子区域掩码（每位代表一个子区域，1=禁用，0=启用）
     * @note 仅当策略提供 get_subregion_mask() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_mpu_get_subregion_mask_method_v<P>>>
    [[nodiscard]] inline static auto get_subregion_mask(typename P::region_index_type region_idx) noexcept {
        return P::get_subregion_mask(region_idx);
    }

    /**
     * @brief 检查两个区域是否重叠
     * @param region_idx1 区域索引1
     * @param region_idx2 区域索引2
     * @return true 如果重叠
     * @note 仅当策略提供 is_overlap() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_mpu_is_overlap_method_v<P>>>
    [[nodiscard]] inline static bool is_overlap(typename P::region_index_type region_idx1,
                                                typename P::region_index_type region_idx2) noexcept {
        static_assert(traits::is_correct_is_overlap_return_v<P>, "is_overlap() must return bool");
        return P::is_overlap(region_idx1, region_idx2);
    }

    /**
     * @brief 返回区域优先级是否与编号顺序一致（编号越大优先级越高）
     * @return 布尔值（通常为 true）
     * @note 仅当策略提供 region_priority_by_number() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_mpu_region_priority_by_number_method_v<P>>>
    [[nodiscard]] inline static auto region_priority_by_number() noexcept {
        return P::region_priority_by_number();
    }
};

} // namespace strat_os::hal

#endif // STRATOS_HAL_MPU_HPP