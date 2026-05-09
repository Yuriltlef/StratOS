/**
 * @file mode.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-04-30
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifndef STRATOS_POLICY_KERNEL_MODE_HPP
#define STRATOS_POLICY_KERNEL_MODE_HPP

namespace strat_os::kernel::policy::builtin
{
struct StaticModePolicy {
    static constexpr bool is_dynamic = false;
};

struct DynamicModePolicy {
    static constexpr bool is_dynamic = true;
};
} // namespace strat_os::kernel::policy::builtin

#endif // STRATOS_POLICY_KERNEL_MODE_HPP