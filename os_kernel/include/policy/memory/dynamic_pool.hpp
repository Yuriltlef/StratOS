/**
 * @file dynamic_pool.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-05-07
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifndef STRATOS_POLICY_KERNEL_DYNAMIC_POOL_HPP
#define STRATOS_POLICY_KERNEL_DYNAMIC_POOL_HPP

#include "os_kernel/include/core/common_traits.hpp"
#include <cstddef> // for std::size_t
#include <cstdint> // for std::uintptr_t, std::uint8_t

namespace strat_os::kernel::policy::builtin
{

template <typename Region, std::size_t BlockSize, std::size_t BlockCount>
struct DynamicPoolPolicy {
    static_assert(::strat_os::kernel::traits::is_region_v<Region>, "Region must be a valid MemoryRegion");

    using region                             = Region;
    using size_type                          = std::size_t;
    using difference_type                    = std::ptrdiff_t;
    using pointer                            = void*;
    using const_pointer                      = const void*;

    static constexpr std::size_t block_size  = BlockSize;
    static constexpr std::size_t block_count = BlockCount;

    static constexpr std::uintptr_t base     = region::layout::base;
    static constexpr std::size_t size        = region::layout::size;
    static constexpr bool is_dynamic         = region::mode::is_dynamic;

    alignas(std::max_align_t) static std::uint8_t pool[block_size * block_count]{};
};

} // namespace strat_os::kernel::policy::builtin

#endif // STRATOS_POLICY_KERNEL_DYNAMIC_POOL_HPP