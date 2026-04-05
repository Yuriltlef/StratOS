/**
 * @file kernel_config.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-04-05
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifndef STRATOS_KERNEL_KERNEL_CONFIG_HPP
#define STRATOS_KERNEL_KERNEL_CONFIG_HPP

#include <cstdint> // for std::uint8_t, std::uint32_t etc.

namespace strat_os::kernel::config
{

struct DefaultKernelConfigPolicy {
    using priority_type = std::uint8_t;  ///< 优先级类型，数值越小优先级越高
    using tick_type     = std::uint32_t; ///< 系统节拍计数类型
    using task_id_type  = std::uint16_t; ///< 任务ID类型
};

} // namespace strat_os::kernel::config

#endif // STRATOS_KERNEL_KERNEL_CONFIG_HPP
