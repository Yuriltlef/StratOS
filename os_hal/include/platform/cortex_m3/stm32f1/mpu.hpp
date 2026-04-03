/**
 * @file mpu.hpp
 * @author StratOS Team
 * @brief 内置 Cortex-M3 MPU 策略（不支持 MPU）
 * @version 1.0.0
 * @date 2026-04-02
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件提供了针对 ARM Cortex-M3 内核的 MPU 策略实现，
 * 但 Cortex-M3 的 MPU 是可选的，本策略声明 is_available = false，
 * 表示该平台不支持 MPU。适配器将因此隐藏所有 MPU 操作接口，
 * 仅提供类型别名和 is_available 常量。
 *
 * 该策略满足 strat_os::hal::traits::is_valid_mpu_policy 的要求
 * （is_available == false 分支只检查常量和类型别名）。
 */
#pragma once

#ifndef STRATOS_HAL_POLICY_CORTEX_M3_STM32F1XX_MPU_HPP
#define STRATOS_HAL_POLICY_CORTEX_M3_STM32F1XX_MPU_HPP

#include <cstdint> // for std::uint32_t, std::uintptr_t

namespace strat_os::hal::policy::builtin
{

/**
 * @brief Cortex-M3 MPU 策略（不支持 MPU）
 *
 * 由于 Cortex-M3 的 MPU 是可选的，且本策略针对无 MPU 的平台，
 * 因此将 is_available 设为 false。适配器将据此隐藏所有 MPU 操作方法，
 * 但提供类型别名以便编译期类型查询。
 */
struct CortexM3Stm32F1MPUPolicy {
    /// 表示该平台不支持 MPU（编译期常量）
    static constexpr bool is_available = false;

    // 占位类型别名（满足接口要求，但实际不使用）
    using region_index_type      = std::uint8_t;   ///< 区域索引类型（占位）
    using region_address_type    = std::uintptr_t; ///< 区域基址类型（占位）
    using region_size_type       = std::uint32_t;  ///< 区域大小类型（占位）
    using region_attributes_type = std::uint32_t;  ///< 区域属性类型（占位）

    /**
     * @brief 区域信息结构体（占位）
     */
    struct region_info_type {
        region_address_type base;    ///< 基址（占位）
        region_size_type size;       ///< 大小（占位）
        region_attributes_type attr; ///< 属性（占位）
        bool enabled;                ///< 使能标志（占位）
    };
};

} // namespace strat_os::hal::policy::builtin

#endif // STRATOS_HAL_POLICY_CORTEX_M3_STM32F1XX_MPU_HPP