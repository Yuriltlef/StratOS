/**
 * @file kernel_types.hpp
 * @author StratOS Team
 * @brief 内核类型默认策略
 * @version 1.1.0
 * @date 2026-04-06
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了 StratOS 内核类型默认策略，采用静态策略模式。
 *
 * @note 该策略类与 HAL 层的策略模式保持一致的风格，便于用户统一理解。
 */
#pragma once

#ifndef STRATOS_POLICY_KERNEL_KERNEL_TYPES_HPP
#define STRATOS_POLICY_KERNEL_KERNEL_TYPES_HPP

#include <cstdint> // for std::uint8_t, std::uint32_t etc.

namespace strat_os::kernel::policy::builtin
{

/**
 * @brief 默认内核类型配置策略
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
struct KernelTypesPolicy {
    /// 优先级类型（数值越小优先级越高，默认 8 位无符号整数）
    using priority_type = std::uint8_t;
    /// 系统节拍计数类型（默认 32 位无符号整数）
    using tick_type = std::uint32_t;
    /// 任务 ID 类型（默认 16 位无符号整数）
    using task_id_type = std::uint16_t;
    /// 任务状态枚举的底层存储类型（默认 8 位无符号整数）
    using task_state_size_type = std::uint8_t;

    /**
     * @brief 默认任务状态枚举
     * @note 必须包含五个标准状态项：Ready, Running, Blocked, Suspended, Terminated
     *       用户可以扩展此枚举，添加自定义状态，但必须保留这五个标准项。
     */
    enum class TaskState : task_state_size_type {
        Ready,     ///< 任务就绪，可被调度
        Running,   ///< 任务正在执行
        Blocked,   ///< 任务被阻塞（等待事件、信号量等）
        Suspended, ///< 任务被挂起（不会参与调度）
        Terminated ///< 任务已终止，等待回收
    };
    /// 任务状态枚举类型别名
    using task_state_type = TaskState;
};

} // namespace strat_os::kernel::policy::builtin

#endif // STRATOS_POLICY_KERNEL_KERNEL_TYPES_HPP