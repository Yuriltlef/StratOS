/**
 * @file memory_layout_type.hpp
 * @author StratOS Team
 * @brief 内存布局类型枚举定义
 * @version 1.0.0
 * @date 2026-06-03
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了 StratOS 支持的内存布局类型枚举。
 * 根据系统配置（纯静态、混合、完全动态），内核会选择不同的内存管理策略。
 *
 * 内存布局类型决定了内存区域的划分方式以及是否使用运行时分配器：
 * - StaticLayout : 所有内存区域在编译期静态划分，无动态分配器。
 * - MixedLayout  : 内核对象静态分配，用户对象动态分配。
 * - DynamicLayout: 整个 RAM 由全局动态分配器管理（两阶段启动）。
 *
 * 该枚举通常作为 `MemoryLayoutConfig::LAYOUT_TYPE` 常量的一部分，
 * 供内核代码在编译期通过 `if constexpr` 进行分支选择。
 */
#pragma once

#ifndef STRATOS_CONFIG_MEMORY_LAYOUT_TYPE_HPP
#define STRATOS_CONFIG_MEMORY_LAYOUT_TYPE_HPP

namespace strat_os::config
{

/**
 * @brief 内存布局类型枚举
 *
 * 用于在编译期区分系统采用的内存管理模型。
 * 不同模型对应不同的链接脚本布局和分配器行为。
 */
enum class MemoryLayoutType {
    /**
     * @brief 纯静态布局
     *
     * 所有内存区域（用户栈、用户池、内核栈、内核池）的大小和地址在编译期完全固定。
     * 不存在任何运行时内存分配器，所有对象均为静态全局对象或放置在固定偏移处。
     * 适用于安全关键系统，行为完全可预测。
     */
    StaticLayout,

    /**
     * @brief 混合布局
     *
     * 内核对象（TCB、队列等）采用静态分配，用户对象（用户栈、动态消息）采用动态分配。
     * 用户池使用动态分配器（如 TLSF），内核池仍为静态。
     * 兼顾确定性与灵活性。
     */
    MixedLayout,

    /**
     * @brief 动态布局
     *
     * 整个 RAM（除分配器元数据外）由全局动态分配器管理。
     * 内核对象和用户对象均从同一堆中分配，需要两阶段启动。
     * 适用于内存需求多变、任务数不固定的场景。
     */
    DynamicLayout
};

} // namespace strat_os::config

#endif // STRATOS_CONFIG_MEMORY_LAYOUT_TYPE_HPP