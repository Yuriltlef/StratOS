/**
 * @file mpu.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-04-02
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifndef STRATOS_HAL_POLICY_CORTEX_M3_MPU_HPP
#define STRATOS_HAL_POLICY_CORTEX_M3_MPU_HPP

#include <cstdint> // for uint32_t, uintptr_t, etc.

namespace strat_os::hal::policy::builtin
{
struct CortexM3MPUPolicy {
    static constexpr bool is_available = false; // Cortex-M3 不支持 MPU
    // 占位类型别名（满足接口要求，但实际不使用）
    using region_index_type      = std::uint8_t;
    using region_address_type    = std::uintptr_t;
    using region_size_type       = std::uint32_t;
    using region_attributes_type = std::uint32_t;
    struct region_info_type {
        region_address_type base;
        region_size_type size;
        region_attributes_type attr;
        bool enabled;
    };
};
} // namespace strat_os::hal::policy::builtin

#endif // STRATOS_HAL_POLICY_CORTEX_M3_MPU_HPP