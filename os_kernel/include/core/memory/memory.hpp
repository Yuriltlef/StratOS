/**
 * @file memory.hpp
 * @author StratOS Team
 * @brief 内存管理策略聚合器（Memory）
 * @version 1.0.0
 * @date 2026-06-05
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件根据内存布局类型（StaticLayout / MixedLayout / DynamicLayout）提供统一的
 * 内存池类型别名（kernel_pool, kernel_stack, user_pool, user_stack）。
 *
 * 内部实现细节位于 `details` 命名空间，包括：
 * - 区域布局常量（基于 MemoryLayoutConfig 从高地址向低地址计算）
 * - 模式标签（KernelStaticMode / KernelDynamicMode）
 * - 线性对象分配器（PoolLinearAllocatorPolicy）
 * - 栈分配器（StackLinearAllocatorPolicy，支持金丝雀）
 *
 * 外部通过 `Memory<LayoutType>` 访问对应布局下的内存池策略。
 */

#pragma once

#ifndef STRATOS_KERNEL_MEMORY_HPP
#define STRATOS_KERNEL_MEMORY_HPP

#include "os_config/include/memory_layout.hpp"
#include "os_config/include/memory_layout_type.hpp"
#include "os_kernel/include/core/memory/memory_pool.hpp"
#include "os_kernel/include/core/memory/region.hpp"
#include <cstddef>
#include <cstdint>

// 链接脚本导出的符号（用于地址一致性验证，保留以备后用）
extern "C" {
extern char _suser_stack[];   // 用户栈区起始
extern char _euser_stack[];   // 用户栈区结束
extern char _suser_pool[];    // 用户池起始
extern char _euser_pool[];    // 用户池结束
extern char _sidata[];        // .data 段的加载地址（Flash）
extern char _sdata[];         // .data 段运行起始（RAM）
extern char _edata[];         // .data 段运行结束
extern char _sbss[];          // .bss 段起始
extern char _ebss[];          // .bss 段结束
extern char _skernel_stack[]; // 内核栈起始
extern char _ekernel_stack[]; // 内核栈结束
extern char _skernel_pool[];  // 内核池起始
extern char _ekernel_pool[];  // 内核池结束
}

namespace strat_os::kernel::details
{

// ==================== 模式标签 ====================
/// 静态模式标签（is_dynamic = false）
struct KernelStaticMode {
    static constexpr bool is_dynamic = false;
};

/// 动态模式标签（is_dynamic = true）
struct KernelDynamicMode {
    static constexpr bool is_dynamic = true;
};

using UserStaticMode  = KernelStaticMode;
using UserDynamicMode = KernelDynamicMode;

// ==================== 区域布局常量（从高地址向低地址依次计算） ====================
/// 内核池布局（最高地址）
struct KernelPoolStaticLayout {
    static constexpr std::uintptr_t base =
        strat_os::config::MemoryLayoutConfig::RAM_END - strat_os::config::MemoryLayoutConfig::KERNEL_POOL_SIZE + 1;
    static constexpr std::size_t size = strat_os::config::MemoryLayoutConfig::KERNEL_POOL_SIZE;
};

/// 内核栈布局（位于内核池下方）
struct KernelStackStaticLayout {
    static constexpr std::uintptr_t base =
        KernelPoolStaticLayout::base - strat_os::config::MemoryLayoutConfig::KERNEL_STACK_SIZE;
    static constexpr std::size_t size = strat_os::config::MemoryLayoutConfig::KERNEL_STACK_SIZE;
};

/// 用户池布局（位于内核栈下方）
struct UserPoolStaticLayout {
    static constexpr std::uintptr_t base =
        KernelStackStaticLayout::base - strat_os::config::MemoryLayoutConfig::USER_POOL_SIZE;
    static constexpr std::size_t size = strat_os::config::MemoryLayoutConfig::USER_POOL_SIZE;
};

/// 用户栈布局（最低地址）
struct UserStackStaticLayout {
    static constexpr std::uintptr_t base =
        UserPoolStaticLayout::base - strat_os::config::MemoryLayoutConfig::USER_STACK_SIZE;
    static constexpr std::size_t size = strat_os::config::MemoryLayoutConfig::USER_STACK_SIZE;
};

// ==================== 内存区域实例化 ====================
using KernelPoolStaticRegion  = MemoryRegion<KernelPoolStaticLayout, KernelStaticMode>;
using KernelStackStaticRegion = MemoryRegion<KernelStackStaticLayout, KernelStaticMode>;
using UserPoolStaticRegion    = MemoryRegion<UserPoolStaticLayout, UserStaticMode>;
using UserStackStaticRegion   = MemoryRegion<UserStackStaticLayout, UserStaticMode>;

// ==================== 线性对象分配器策略（用于内核池/用户池） ====================
/**
 * @brief 线性分配器策略（从低地址向高地址顺序分配，不释放）
 * @tparam LayoutPolicy 布局策略（提供 base 和 size）
 * @tparam ModePolicy   模式策略（提供 is_dynamic）
 *
 * 该分配器适用于静态内存池（内核池、用户池），分配固定大小的对象块。
 * 所有分配按请求大小对齐到 alignof(std::max_align_t)，返回内存块起始地址。
 * 不支持释放（deallocate 为空），但提供 reset() 重置分配位置。
 */
template <typename LayoutPolicy, typename ModePolicy>
struct PoolLinearAllocatorPolicy {
    using region                         = MemoryRegion<LayoutPolicy, ModePolicy>;
    static constexpr std::uintptr_t base = region::layout::base;
    static constexpr std::size_t size    = region::layout::size;
    static constexpr bool is_dynamic     = false;

    using size_type                      = std::size_t;
    using difference_type                = std::ptrdiff_t;
    using pointer                        = void*;
    using const_pointer                  = const void*;

  private:
    static inline std::uintptr_t free_list = base; ///< 下一个空闲地址
    static constexpr std::size_t alignment = alignof(std::max_align_t);

    /// 将大小向上对齐到 alignment 的倍数
    [[nodiscard]] static constexpr std::size_t align_up(std::size_t n) noexcept {
        return (n + alignment - 1) & ~(alignment - 1);
    }

  public:
    /**
     * @brief 分配一块内存
     * @param alloc_size 请求大小（字节）
     * @return 分配的内存起始地址，空间不足返回 nullptr
     */
    [[nodiscard]] static pointer allocate(std::size_t alloc_size) noexcept {
        alloc_size              = align_up(alloc_size);
        std::uintptr_t new_free = free_list + static_cast<std::uintptr_t>(alloc_size);
        if (new_free > base + size) {
            return nullptr;
        }
        free_list = new_free;
        return reinterpret_cast<pointer>(free_list - alloc_size);
    }

    /// 释放（线性分配器不支持单个释放）
    static void deallocate(pointer /*ptr*/) noexcept {}

    /// 重置分配器（使内存区域可重新使用）
    static void reset() noexcept {
        free_list = base;
    }
};

// ==================== 线性栈分配器策略（支持金丝雀） ====================
/**
 * @brief 线性栈分配器策略，从低地址向高地址顺序分配任务栈，返回栈顶地址
 * @tparam LayoutPolicy 布局策略（提供 base 和 size）
 * @tparam ModePolicy   模式策略（提供 is_dynamic）
 * @tparam UseCanary    是否在栈底放置金丝雀值（默认 true）
 *
 * 分配的单位为：对齐后的栈大小 + 可选金丝雀大小。每个块结构：
 *   [ 金丝雀(4字节, 可选) ] [ 实际栈空间(用户请求大小, 对齐) ]
 * 返回地址为栈顶（即块起始 + 总大小），适合直接赋值给 TCB 的 sp 指针。
 * 金丝雀值位于栈底（最低地址），可检测栈下溢（栈向下增长时溢出会覆盖金丝雀）。
 */
template <typename LayoutPolicy, typename ModePolicy, bool UseCanary = true>
struct StackLinearAllocatorPolicy {
    using region                             = MemoryRegion<LayoutPolicy, ModePolicy>;
    static constexpr std::uintptr_t base     = region::layout::base;
    static constexpr std::size_t size        = region::layout::size;
    static constexpr bool use_canary         = UseCanary;
    static constexpr std::size_t canary_size = use_canary ? sizeof(std::uint32_t) : 0;
    static constexpr bool is_dynamic         = false;

    using size_type                          = std::size_t;
    using difference_type                    = std::ptrdiff_t;
    using pointer                            = void*;
    using const_pointer                      = const void*;

  private:
    static inline std::uintptr_t next_free = base; ///< 下一个空闲块的起始地址（栈底）
    static constexpr std::size_t alignment = alignof(std::max_align_t);

    /// 向上对齐到 alignment 的倍数
    static constexpr std::size_t align_up(std::size_t n) noexcept {
        return (n + alignment - 1) & ~(alignment - 1);
    }

  public:
    /**
     * @brief 分配一个任务栈
     * @param stack_size 用户请求的栈大小（字节）
     * @return 栈顶地址（高地址），空间不足返回 nullptr
     */
    [[nodiscard]] static pointer allocate(std::size_t stack_size) noexcept {
        std::size_t aligned_stack    = align_up(stack_size);
        std::size_t total_size       = aligned_stack + canary_size;

        std::uintptr_t block_start   = next_free;
        std::uintptr_t new_next_free = block_start + total_size;
        if (new_next_free > base + size) {
            return nullptr; // 空间不足
        }

        // 放置金丝雀（如果启用）
        if constexpr (use_canary) {
            *reinterpret_cast<std::uint32_t*>(block_start) = 0xDEADBEEF;
        }

        next_free = new_next_free;
        // 返回栈顶地址（块起始 + 总大小）
        return reinterpret_cast<pointer>(block_start + total_size);
    }

    /// 释放（线性分配器不支持单个栈释放）
    static void deallocate(pointer /*ptr*/) noexcept {}

    /// 重置分配器
    static void reset() noexcept {
        next_free = base;
    }
};

} // namespace strat_os::kernel::details

namespace strat_os::kernel
{

/**
 * @brief 内存策略族，根据内存布局类型提供对应的内存池类型别名
 * @tparam LayoutType 布局类型（StaticLayout / MixedLayout / DynamicLayout）
 *
 * 用户通过 `Memory<LayoutType>::kernel_pool` 等类型别名获取分配器。
 */
template <strat_os::config::MemoryLayoutType LayoutType>
struct Memory;

// ==================== 纯静态模式特化 ====================
/**
 * @brief 纯静态模式内存策略
 *
 * 四个独立区域均使用线性分配器（不支持释放），符合确定性要求。
 */
template <>
struct Memory<strat_os::config::MemoryLayoutType::StaticLayout> {
    /// 内核池分配器（用于 TCB、队列等内核对象）
    using kernel_pool =
        MemoryPool<details::PoolLinearAllocatorPolicy<details::KernelPoolStaticLayout, details::KernelStaticMode>>;

    /// 内核栈分配器（实际只分配一次，但接口统一）
    using kernel_stack = MemoryPool<details::StackLinearAllocatorPolicy<details::KernelStackStaticLayout,
                                                                        details::KernelStaticMode,
                                                                        false>>; // 内核栈无需金丝雀

    /// 用户池分配器（用户静态对象）
    using user_pool =
        MemoryPool<details::PoolLinearAllocatorPolicy<details::UserPoolStaticLayout, details::UserStaticMode>>;

    /// 用户栈分配器（任务栈，启用金丝雀）
    using user_stack =
        MemoryPool<details::StackLinearAllocatorPolicy<details::UserStackStaticLayout, details::UserStaticMode, true>>;
};

// ==================== 混合模式特化（待实现） ====================
template <>
struct Memory<strat_os::config::MemoryLayoutType::MixedLayout> {
    // TODO: 内核静态 + 用户动态的组合
};

// ==================== 完全动态模式特化（待实现） ====================
template <>
struct Memory<strat_os::config::MemoryLayoutType::DynamicLayout> {
    // TODO: 单一全局动态分配器
};

} // namespace strat_os::kernel

#endif // STRATOS_KERNEL_MEMORY_HPP