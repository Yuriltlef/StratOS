/**
 * @file static_pool.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-06-04
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "os_kernel/include/policy/memory/static_pool.hpp"
#include "os_config/include/memory_layout.hpp"

namespace strat_os::kernel::policy::builtin
{
alignas(std::max_align_t) __attribute__((section(".kernel_pool"))) std::uint8_t
    static_kernel_pool_memory[strat_os::config::MemoryLayoutConfig::KERNEL_POOL_SIZE]{};
} // namespace strat_os::kernel::policy::builtin