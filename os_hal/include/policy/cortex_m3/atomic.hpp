/**
 * @file atomic.hpp
 * @author StratOS Team
 * @brief 内置 Cortex-M3 原子操作策略
 * @version 1.0.0
 * @date 2026-04-01
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件提供了针对 ARM Cortex-M3 内核的原子操作策略实现，
 * 属于官方内置策略（builtin）。它基于 GCC 内置原子函数
 * （__atomic_*）实现了原子加载、存储、算术运算、比较交换
 * 以及位操作，并支持可选的 std::memory_order 参数。
 *
 * 该策略满足 strat_os::hal::traits::is_valid_atomic_policy
 * 的所有要求，可直接与 strat_os::hal::Atomic 适配器配合使用。
 *
 * @note 使用 GCC 内置原子函数，在 Cortex-M3 上会生成最优的
 *       LDREX/STREX 循环和内存屏障，保证零开销抽象。
 * @warning 所有方法均为 noexcept，适合在实时环境中使用。
 */
#pragma once

#ifndef STRATOS_HAL_POLICY_CORTEX_M3_ATOMIC_HPP
#define STRATOS_HAL_POLICY_CORTEX_M3_ATOMIC_HPP

#include "stm32f10x.h" // for IRQn_Type (not used directly, but required by CMSIS)
#include <atomic>      // for std::memory_order
#include <cstdint>     // for uint32_t, uint8_t

#ifndef __CM3_CORE_H__
#include "core_cm3.h"
#endif

namespace
{
/// @warning 仅用于避免 clangd 对未使用 stm32f10x.h 的警告（实际不使用）
using IQRN_TYPE_KEEP_FOR_NO_CLANGD_WARNING_AND_CMSIS = ::IRQn_Type;
};

namespace strat_os::hal::policy::builtin {

/**
 * @brief Cortex-M3 原子操作内置策略
 *
 * 该结构体封装了 Cortex-M3 上的原子操作，通过 GCC 内置原子函数实现，
 * 提供与 strat_os::hal::Atomic 适配器完全兼容的静态接口。
 * 所有方法均为内联且 noexcept，保证零开销抽象。
 */
struct CortexM3AtomicPolicy {
    /// 原子操作的基本类型（无符号 32 位整数）
    using value_type = std::uint32_t;
    /// 位索引类型（无符号 8 位整数）
    using bit_index_type = std::uint8_t;

    // ========== 必需方法（无内存顺序） ==========

    /**
     * @brief 原子加载（顺序一致性）
     * @param ptr 指向 volatile 内存的指针
     * @return 当前值
     */
    static value_type load(volatile value_type* ptr) noexcept {
        return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
    }

    /**
     * @brief 原子存储（宽松顺序）
     * @param ptr   指向 volatile 内存的指针
     * @param value 要存储的值
     */
    static void store(volatile value_type* ptr, value_type value) noexcept {
        __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
    }

    /**
     * @brief 原子加法（宽松顺序）
     * @param ptr   指向 volatile 内存的指针
     * @param value 要增加的值
     * @return 加后的新值
     */
    static value_type add(volatile value_type* ptr, value_type value) noexcept {
        return __atomic_add_fetch(ptr, value, __ATOMIC_RELAXED);
    }

    /**
     * @brief 原子减法（宽松顺序）
     * @param ptr   指向 volatile 内存的指针
     * @param value 要减少的值
     * @return 减后的新值
     */
    static value_type sub(volatile value_type* ptr, value_type value) noexcept {
        return __atomic_sub_fetch(ptr, value, __ATOMIC_RELAXED);
    }

    /**
     * @brief 原子比较交换（宽松顺序）
     * @param ptr      指向 volatile 内存的指针
     * @param expected 期望值（若匹配则替换；若失败，该引用会被更新为当前值）
     * @param desired  目标值
     * @return true  如果成功写入
     * @return false 如果期望值不匹配（此时 *ptr 的值会写入 expected）
     */
    static bool compare_exchange(volatile value_type* ptr,
                                 value_type& expected,
                                 value_type desired) noexcept {
        return __atomic_compare_exchange_n(ptr,
                                           &expected,
                                           desired,
                                           false, // 弱交换（强交换也可）
                                           __ATOMIC_RELAXED,
                                           __ATOMIC_RELAXED);
    }

    /**
     * @brief 原子地设置指定位为 1（宽松顺序）
     * @param ptr 指向 volatile 内存的指针
     * @param bit 位索引
     */
    static void set_bit(volatile value_type* ptr, bit_index_type bit) noexcept {
        __atomic_fetch_or(ptr, static_cast<value_type>(1) << bit, __ATOMIC_RELAXED);
    }

    /**
     * @brief 原子地清除指定位为 0（宽松顺序）
     * @param ptr 指向 volatile 内存的指针
     * @param bit 位索引
     */
    static void clear_bit(volatile value_type* ptr, bit_index_type bit) noexcept {
        __atomic_fetch_and(ptr, ~(static_cast<value_type>(1) << bit), __ATOMIC_RELAXED);
    }

    /**
     * @brief 原子地测试并设置指定位（返回旧值，并置为 1）
     * @param ptr 指向 volatile 内存的指针
     * @param bit 位索引
     * @return true  原值为 1
     * @return false 原值为 0
     */
    static bool test_and_set_bit(volatile value_type* ptr, bit_index_type bit) noexcept {
        value_type old = __atomic_fetch_or(ptr, static_cast<value_type>(1) << bit, __ATOMIC_RELAXED);
        return static_cast<bool>((old >> bit) & 1);
    }

    /**
     * @brief 原子地翻转指定位（宽松顺序）
     * @param ptr 指向 volatile 内存的指针
     * @param bit 位索引
     */
    static void flip_bit(volatile value_type* ptr, bit_index_type bit) noexcept {
        __atomic_fetch_xor(ptr, static_cast<value_type>(1) << bit, __ATOMIC_RELAXED);
    }

    /**
     * @brief 原子测试并设置整个字（返回旧值，并置为 1）
     * @param ptr 指向 volatile 内存的指针
     * @return true  原值为 1
     * @return false 原值为 0
     * @note 常用于自旋锁的实现。
     */
    static bool test_and_set(volatile value_type* ptr) noexcept {
        value_type old = __atomic_exchange_n(ptr, 1, __ATOMIC_RELAXED);
        return old != 0;
    }

    // ========== 可选扩展方法（带内存顺序） ==========

    /**
     * @brief 原子加载（带内存顺序）
     * @param ptr   指向 volatile 内存的指针
     * @param order 内存顺序（如 std::memory_order_acquire）
     * @return 当前值
     */
    static value_type load(volatile value_type* ptr, std::memory_order order) noexcept {
        return __atomic_load_n(ptr, order);
    }

    /**
     * @brief 原子存储（带内存顺序）
     * @param ptr   指向 volatile 内存的指针
     * @param value 要存储的值
     * @param order 内存顺序（如 std::memory_order_release）
     */
    static void store(volatile value_type* ptr, value_type value, std::memory_order order) noexcept {
        __atomic_store_n(ptr, value, order);
    }

    /**
     * @brief 原子加法（带内存顺序）
     * @param ptr   指向 volatile 内存的指针
     * @param value 要增加的值
     * @param order 内存顺序
     * @return 加后的新值
     */
    static value_type add(volatile value_type* ptr, value_type value, std::memory_order order) noexcept {
        return __atomic_add_fetch(ptr, value, order);
    }

    /**
     * @brief 原子减法（带内存顺序）
     * @param ptr   指向 volatile 内存的指针
     * @param value 要减少的值
     * @param order 内存顺序
     * @return 减后的新值
     */
    static value_type sub(volatile value_type* ptr, value_type value, std::memory_order order) noexcept {
        return __atomic_sub_fetch(ptr, value, order);
    }

    /**
     * @brief 原子比较交换（带内存顺序）
     * @param ptr      指向 volatile 内存的指针
     * @param expected 期望值（若匹配则替换；若失败，该引用会被更新为当前值）
     * @param desired  目标值
     * @param order    内存顺序（成功和失败使用相同顺序）
     * @return true  如果成功写入
     * @return false 如果期望值不匹配
     */
    static bool compare_exchange(volatile value_type* ptr,
                                 value_type& expected,
                                 value_type desired,
                                 std::memory_order order) noexcept {
        return __atomic_compare_exchange_n(ptr, &expected, desired, false, order, order);
    }
};

} // namespace strat_os::hal::policy::builtin

#endif // STRATOS_HAL_POLICY_CORTEX_M3_ATOMIC_HPP