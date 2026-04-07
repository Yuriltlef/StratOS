/**
 * @file kernel_config.hpp
 * @author StratOS Team
 * @brief 内核配置策略接口
 * @version 1.1.0
 * @date 2026-04-06
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了 StratOS 内核的配置策略接口，采用静态策略模式。
 * 用户可以通过自定义策略类来替换内核的基础类型（优先级、节拍计数、任务ID、任务状态等），
 * 从而适应不同应用场景的需求（如需要更多优先级级别、更长的时间范围、自定义任务状态等）。
 *
 * 策略类必须定义以下嵌套类型：
 * - priority_type：优先级类型（通常为无符号整数，数值越小优先级越高）
 * - tick_type：系统节拍计数类型（通常为无符号整数，决定系统能运行的最大节拍数）
 * - task_id_type：任务ID类型（通常为无符号整数，决定系统支持的最大任务数）
 * - task_state_size_type：任务状态枚举的底层存储类型（无符号整数，至少8位）
 * - task_state_type：任务状态枚举类，必须包含 Ready、Running、Blocked、Suspended、Terminated 五个枚举项
 *
 * 使用示例：
 * @code
 * // 自定义内核配置（支持 256 个优先级，64 位节拍，最多 65535 个任务，自定义任务状态）
 * struct MyConfig {
 *     using priority_type = uint8_t;
 *     using tick_type     = uint64_t;
 *     using task_id_type  = uint16_t;
 *     using task_state_size_type = uint8_t;
 *     enum class task_state_type : task_state_size_type {
 *         Ready, Running, Blocked, Suspended, Terminated,
 *         WaitingForIO   // 扩展状态
 *     };
 * };
 *
 * // 在内核中使用
 * using KernelTypes = strat_os::kernel::KernelTypes<MyConfig>;
 * @endcode
 *
 * @note 该策略类与 HAL 层的策略模式保持一致的风格，便于用户统一理解。
 * @warning 修改这些类型可能影响内核的行为（例如优先级范围、系统最大运行时间），
 *          请确保修改后的类型与底层硬件和调度器实现兼容。
 *          自定义任务状态枚举时，必须包含五个标准状态项（名称精确匹配），否则编译失败。
 */
#pragma once

#ifndef STRATOS_KERNEL_KERNEL_CONFIG_HPP
#define STRATOS_KERNEL_KERNEL_CONFIG_HPP

#include "os_kernel/include/core/types.hpp"                // for KernelTypes
#include "os_kernel/include/policy/types/kernel_types.hpp" // for KernelTypesPolicy
#include "os_kernel/include/policy/types/tcb.hpp"          // for UserTcbDataPolicy


namespace strat_os::kernel::config
{

/**
 * @brief 默认内核配置策略
 *
 * 该策略提供了内核所需的五种基础类型的默认定义：
 * - priority_type：uint8_t（支持 0～255 共 256 个优先级）
 * - tick_type：uint32_t（支持最长约 49 天的节拍计数，假设 1ms 节拍）
 * - task_id_type：uint16_t（支持最多 65535 个任务）
 * - task_state_size_type：uint8_t（任务状态枚举的底层存储类型）
 * - task_state_type：TaskState 枚举，包含 Ready、Running、Blocked、Suspended、Terminated 五个标准状态
 *
 * 用户可以直接使用此默认策略，或通过自定义策略类覆盖这些类型。
 */
using DefaultKernelConfigPolicy = strat_os::kernel::policy::builtin::KernelTypesPolicy;

using DefaultKernelConfig       = strat_os::kernel::KernelTypes<DefaultKernelConfigPolicy>;

using DefaultUserTcbDataPolicy  = strat_os::kernel::policy::builtin::UserTcbDataPolicy;

} // namespace strat_os::kernel::config

#endif // STRATOS_KERNEL_KERNEL_CONFIG_HPP