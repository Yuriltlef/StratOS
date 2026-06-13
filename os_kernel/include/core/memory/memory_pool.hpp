/**
 * @file memory_pool.hpp
 * @author StratOS Team
 * @brief 内存池策略适配器（MemoryPool），要求策略提供合法 region
 * @version 1.0.0
 * @date 2026-04-29
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了内存池（MemoryPool）的策略适配器。内存池负责实际的内存分配与释放。
 *
 * 内存池策略类（MemoryPoolPolicy）必须提供以下静态方法：
 * - void* allocate(std::size_t size) noexcept
 * - void deallocate(void* ptr) noexcept
 *
 * 同时，策略类必须提供一个嵌套类型 `region`，该类型必须是 `MemoryRegion` 的实例
 * （即通过 `traits::is_region_v` 检测）。这使得内存池能够获取关联内存区域的元数据
 * （如布局的 base/size，模式的 is_dynamic），实现编译期策略组合。
 *
 * 可选增强功能（通过 SFINAE 检测）：
 * - void init() noexcept
 * - std::size_t get_total_size() noexcept
 * - std::size_t get_used_size() noexcept
 * - void reset() noexcept
 *
 * 适配器模板 `MemoryPool` 会进行编译期验证，确保策略满足接口要求，
 * 并转发所有调用。可选方法只有在策略提供时才存在于适配器中。
 * @par 使用示例
 * @code
 * // 定义静态池策略
 * template<typename Region, size_t BlockSize, size_t BlockCount>
 * struct StaticPoolPolicy {
 *     alignas(BlockSize) static inline uint8_t storage[BlockSize * BlockCount];
 *     static inline void* free_list = storage;
 *
 *     static void* allocate(size_t size) noexcept {
 *         if (size > BlockSize) return nullptr;
 *         if (!free_list) return nullptr;
 *         void* p = free_list;
 *         free_list = *reinterpret_cast<void**>(p);
 *         return p;
 *     }
 *     static void deallocate(void* p) noexcept {
 *         if (!p) return;
 *         *reinterpret_cast<void**>(p) = free_list;
 *         free_list = p;
 *     }
 *     static constexpr size_t get_total_size() noexcept { return BlockSize * BlockCount; }
 * };
 *
 * // 使用适配器
 * using MyLayout = MemoryLayout<MyLayoutPolicy>;
 * using MyMode   = MemoryMode<MyModePolicy>;
 * using MyRegion = MemoryRegion<MyLayout, MyMode>;
 * using MyPolicy = StaticPoolPolicy<MyRegion, 64, 128>;
 * using MyPool   = MemoryPool<MyPolicy>;
 *
 * void* mem = MyPool::allocate(32);
 * MyPool::deallocate(mem);
 * @endcode
 */
#pragma once

#ifndef STRATOS_KERNEL_MEMORY_POOL_HPP
#define STRATOS_KERNEL_MEMORY_POOL_HPP

#include "os_kernel/include/core/common_traits.hpp"
#include <cstddef> // for std::size_t
#include <type_traits>

namespace strat_os::kernel::traits
{
/**
 * @brief 检测类型 T 是否提供嵌套类型 region
 */
template <typename T, typename = void>
struct has_pool_region : std::false_type {};

template <typename T>
struct has_pool_region<T, std::void_t<typename T::region>> : std::true_type {};

template <typename T>
static constexpr bool has_pool_region_v = has_pool_region<T>::value;

// ----- 必需方法检测 -----

/**
 * @brief 检测类型 T 是否提供静态方法 allocate(size_t)
 */
template <typename T, typename = void>
struct has_pool_allocate : std::false_type {};

template <typename T>
struct has_pool_allocate<T, std::void_t<decltype(T::allocate(std::declval<std::size_t>()))>> : std::true_type {};

template <typename T>
static constexpr bool has_pool_allocate_v = has_pool_allocate<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 deallocate(void*)
 */
template <typename T, typename = void>
struct has_pool_deallocate : std::false_type {};

template <typename T>
struct has_pool_deallocate<T, std::void_t<decltype(T::deallocate(std::declval<void*>()))>> : std::true_type {};

template <typename T>
static constexpr bool has_pool_deallocate_v = has_pool_deallocate<T>::value;

/**
 * @brief 组合检测：必需方法是否存在
 */
template <typename T>
struct has_required_pool_methods : std::conjunction<has_pool_allocate<T>, has_pool_deallocate<T>> {};

template <typename T>
static constexpr bool has_required_pool_methods_v = has_required_pool_methods<T>::value;

// ----- 可选方法检测 -----

/**
 * @brief 检测类型 T 是否提供静态方法 init()
 */
template <typename T, typename = void>
struct has_pool_init : std::false_type {};

template <typename T>
struct has_pool_init<T, std::void_t<decltype(T::init())>> : std::true_type {};

template <typename T>
static constexpr bool has_pool_init_v = has_pool_init<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 get_total_size() -> std::size_t
 */
template <typename T, typename = void>
struct has_pool_get_total_size : std::false_type {};

template <typename T>
struct has_pool_get_total_size<T, std::void_t<decltype(T::get_total_size())>>
    : std::is_same<decltype(T::get_total_size()), std::size_t> {};

template <typename T>
static constexpr bool has_pool_get_total_size_v = has_pool_get_total_size<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 get_used_size() -> std::size_t
 */
template <typename T, typename = void>
struct has_pool_get_used_size : std::false_type {};

template <typename T>
struct has_pool_get_used_size<T, std::void_t<decltype(T::get_used_size())>>
    : std::is_same<decltype(T::get_used_size()), std::size_t> {};

template <typename T>
static constexpr bool has_pool_get_used_size_v = has_pool_get_used_size<T>::value;

/**
 * @brief 检测类型 T 是否提供静态方法 reset()
 */
template <typename T, typename = void>
struct has_pool_reset : std::false_type {};

template <typename T>
struct has_pool_reset<T, std::void_t<decltype(T::reset())>> : std::true_type {};

template <typename T>
static constexpr bool has_pool_reset_v = has_pool_reset<T>::value;

// ----- 整体有效性检测 -----

/**
 * @brief 组合检测：判断类型 T 是否为有效的内存池策略
 *
 * 要求 T 必须提供 allocate(size_t) 和 deallocate(void*) 静态方法。
 * 可选方法（init, get_total_size, get_used_size, reset）根据需要单独检测。
 */
template <typename T>
struct is_valid_pool_policy
    : std::conjunction<has_required_pool_methods<T>, has_pool_region<T>, is_region<typename T::region>> {};

template <typename T>
static constexpr bool is_valid_pool_policy_v = is_valid_pool_policy<T>::value;

} // namespace strat_os::kernel::traits

namespace strat_os::kernel
{

/**
 * @brief 内存池适配器模板
 * @tparam MemoryPoolPolicy 具体的池策略类，必须提供 allocate 和 deallocate 静态方法
 *
 * 该类将池策略包装为统一的静态接口，并进行编译期验证。
 * 所有方法均为内联且 noexcept，转发到策略类的对应静态方法。
 *
 * 若策略支持可选功能（如 init、内存统计、重置等），则会通过 SFINAE
 * 提供相应重载；否则这些方法在适配器中不可用，避免代码膨胀。
 *
 * @note 池策略类通常需要知道内存区域的布局信息（基址、大小）以及分配算法细节。
 *       这些信息可以通过模板参数传递给策略类，但适配器只要求策略类公开
 *       指定的静态方法，不关心其内部实现。
 *
 * @warning 用户不得直接实例化此模板；所有方法均为静态，应通过类型别名使用。
 */
template <typename MemoryPoolPolicy, typename = std::enable_if_t<traits::is_valid_pool_policy_v<MemoryPoolPolicy>>>
struct MemoryPool {
    /// 原始池策略类型
    using Policy          = MemoryPoolPolicy;

    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer         = void*;
    using const_pointer   = const void*;

    // ----- 细粒度静态断言，提供清晰的错误信息 -----
    static_assert(traits::has_pool_region_v<MemoryPoolPolicy>, "MemoryPoolPolicy must define nested type 'region'");
    static_assert(traits::is_region_v<typename MemoryPoolPolicy::region>,
                  "Policy::region must be a valid MemoryRegion instantiation (detected by traits::is_region_v)");
    static_assert(traits::has_pool_allocate_v<Policy>,
                  "MemoryPoolPolicy must provide 'allocate(size_t)' static method");
    static_assert(traits::has_pool_deallocate_v<Policy>,
                  "MemoryPoolPolicy must provide 'deallocate(void*)' static method");

    /// 关联的内存区域类型
    using region = typename Policy::region;

    /**
     * @brief 从内存池分配一块内存
     * @param size 请求的字节数
     * @return 指向分配内存的指针，失败返回 nullptr
     * @note 分配的内存地址至少对齐到 alignof(std::max_align_t)
     */
    [[nodiscard]] inline static void* allocate(std::size_t size) noexcept {
        return Policy::allocate(size);
    }

    /**
     * @brief 释放之前分配的内存块
     * @param ptr 指向要释放的内存块的指针，可以为 nullptr
     * @note 行为与标准 free 类似：传递 nullptr 无效果。
     */
    inline static void deallocate(void* ptr) noexcept {
        Policy::deallocate(ptr);
    }

    // ----- 可选扩展方法（通过 SFINAE 检测并转发）-----

    /**
     * @brief 初始化内存池（如果需要）
     * @note 某些分配器（如 TLSF）需要在使用前初始化内部数据结构。
     *       仅当策略提供 `init()` 方法时可用。
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_pool_init_v<P>>>
    inline static void init() noexcept {
        P::init();
    }

    /**
     * @brief 获取内存池的总大小（字节）
     * @return 池的总容量
     * @note 仅当策略提供 `get_total_size()` 方法时可用。
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_pool_get_total_size_v<P>>>
    [[nodiscard]] inline static std::size_t get_total_size() noexcept {
        return P::get_total_size();
    }

    /**
     * @brief 获取当前已分配的内存大小（字节）
     * @return 已分配的总字节数
     * @note 仅当策略提供 `get_used_size()` 方法时可用。
     *       可用于内存使用率监控。
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_pool_get_used_size_v<P>>>
    [[nodiscard]] inline static std::size_t get_used_size() noexcept {
        return P::get_used_size();
    }

    /**
     * @brief 重置内存池到初始状态（清空所有分配）
     * @note 仅当策略提供 `reset()` 方法时可用。
     *       通常在系统热重启或测试场景中使用。
     * @warning 重置后所有之前分配的内存将变为无效，必须确保
     *          没有悬挂指针再使用这些内存。
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_pool_reset_v<P>>>
    inline static void reset() noexcept {
        P::reset();
    }
};

} // namespace strat_os::kernel

#endif // STRATOS_KERNEL_MEMORY_POOL_HPP