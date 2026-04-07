/**
 * @file tcb.hpp
 * @author StratOS Team
 * @brief 任务控制块（TCB）策略适配器定义
 * @version 1.0.0
 * @date 2026-04-07
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了任务控制块（TCB）的策略适配器。TCB 是内核调度器的核心数据结构，
 * 用于存储每个任务的上下文、优先级、状态、入口函数等信息。
 *
 * TCB 被设计为模板结构体，依赖三个策略：
 * - KernelConfigPolicy：内核配置策略（优先级类型、节拍类型、任务ID类型、任务状态枚举等）
 * - PlatformContextPolicy：平台上下文策略（FPU 寄存器、MPU 配置等）
 * - UserTcbDataPolicy：用户数据扩展策略（自定义任务字段，如名称、截止时间）
 *
 * 该设计符合 StratOS 的静态策略模式，用户可以通过自定义策略来扩展 TCB，
 * 而无需修改内核代码。平台上下文和用户数据通过多重继承嵌入 TCB，
 * 空基类优化确保无额外内存开销。
 *
 * @note TCB 的分配方式（静态数组、动态池）由上层任务管理器策略决定，
 *       TCB 本身只负责数据存储，不关心分配逻辑。
 */
#pragma once

#ifndef STRATOS_KERNEL_TCB_HPP
#define STRATOS_KERNEL_TCB_HPP

#include "os_hal/include/platform_context.hpp" // for PlatformContext
#include "os_kernel/include/core/types.hpp"    // for KernelTypes, UserTcbData
#include <cstdint>                             // for uintptr_t
#include <type_traits>                         // for std::enable_if_t
#include <utility>                             // for std::forward

namespace strat_os::kernel
{

/**
 * @brief 标准任务控制块（TCB）基础结构
 * @tparam KernelConfigPolicy 内核配置策略（提供 priority_type, task_id_type, task_state_type 等）
 *
 * 该结构体包含调度器必需的标准字段，不涉及任何平台或用户扩展。
 * 它是所有 TCB 变体的公共基础部分。
 */
template <typename KernelConfigPolicy>
struct TcbStandard {
    // 导入内核类型别名
    using Types           = KernelTypes<KernelConfigPolicy>;
    using priority_type   = typename Types::priority_type;   ///< 优先级类型
    using task_id_type    = typename Types::task_id_type;    ///< 任务 ID 类型
    using task_state_type = typename Types::task_state_type; ///< 任务状态枚举类型

    /// 栈指针（由汇编代码读写，指向当前任务栈顶）
    std::uintptr_t sp{};

    /// 任务入口函数指针（无参数）
    void (*entry)(){nullptr};

    /// 任务优先级（数值越小优先级越高，具体解释由调度策略决定）
    priority_type priority{};

    /// 任务唯一标识符
    task_id_type id{};

    /// 任务当前状态（就绪、运行、阻塞等）
    task_state_type state{};

    /// 链表指针（用于就绪队列或阻塞队列；若使用位图或数组可省略）
    TcbStandard* next{nullptr};

    /**
     * @brief 默认构造函数，零初始化所有成员
     * @note 静态分配时编译器自动零初始化；动态分配时需配合值初始化。
     */
    TcbStandard() = default;

    /**
     * @brief 构造函数，便于创建任务时初始化标准字段
     * @param entry_func 任务入口函数
     * @param prio       优先级
     * @param task_id    任务 ID
     */
    constexpr TcbStandard(void (*entry_func)(), priority_type prio, task_id_type task_id) noexcept
        : sp(static_cast<std::uintptr_t>(0))
        , entry(entry_func)
        , priority(prio)
        , id(task_id)
        , state(task_state_type::Ready)
        , next(nullptr) {}

    // 禁止拷贝和移动（任务不应被复制或转移所有权）
    TcbStandard(const TcbStandard&)            = delete;
    TcbStandard& operator=(const TcbStandard&) = delete;
    TcbStandard(TcbStandard&&)                 = delete;
    TcbStandard& operator=(TcbStandard&&)      = delete;
};

/**
 * @brief 任务控制块（TCB）最终适配器模板
 * @tparam KernelConfigPolicy     内核配置策略
 * @tparam PlatformContextPolicy  平台上下文策略
 * @tparam UserTcbDataPolicy      用户数据扩展策略
 *
 * 该类通过多重继承组合标准字段、平台上下文和用户数据。
 * - 标准字段继承自 TcbStandard
 * - 平台上下文继承自 PlatformContextPolicy 定义的 platform_context_type
 * - 用户数据继承自 UserTcbDataPolicy 定义的 user_data_type
 *
 * 如果平台上下文或用户数据为空类型，空基类优化会消除其空间占用。
 *
 * @note 用户数据扩展策略必须提供 `user_data_type` 类型和 `supports_user_data` 常量。
 *       平台上下文策略必须提供 `platform_context_type` 和 `supports_platform_context` 常量。
 */
template <typename KernelConfigPolicy, typename PlatformContextPolicy, typename UserTcbDataPolicy>
struct Tcb : public TcbStandard<KernelConfigPolicy>,
             public strat_os::hal::PlatformContext<PlatformContextPolicy>::platform_context_type,
             public UserTcbData<UserTcbDataPolicy>::user_data_type {

    /// 平台上下文适配器类型别名
    using platform_context = strat_os::hal::PlatformContext<PlatformContextPolicy>;
    /// 标准 TCB 类型别名
    using st_tcb_type = TcbStandard<KernelConfigPolicy>;
    /// 用户数据适配器类型别名
    using user_tcb_data = UserTcbData<UserTcbDataPolicy>;

    /// 平台上下文类型（可能为空）
    using platform_context_type = typename platform_context::platform_context_type;
    /// 用户数据类型（可能为空）
    using user_data_type = typename user_tcb_data::user_data_type;

    /// 栈指针类型
    using sp_type = std::uintptr_t;
    /// 优先级类型
    using priority_type = typename st_tcb_type::priority_type;
    /// 任务 ID 类型
    using task_id_type = typename st_tcb_type::task_id_type;
    /// 任务状态类型
    using task_state_type = typename st_tcb_type::task_state_type;

    /**
     * @brief 构造函数（无用户数据扩展）
     * @param sp   栈指针
     * @param prio 优先级
     * @note 仅当用户数据扩展为空或用户不需要传递初始化参数时使用。
     */
    explicit Tcb(void (*entry_func)(), priority_type prio, task_id_type task_id)
        : st_tcb_type{entry_func, prio, task_id} {}

    /**
     * @brief 构造函数（带用户数据扩展参数）
     * @tparam ExtArgs 用户数据扩展类型的构造参数类型包
     * @param sp       栈指针
     * @param prio     优先级
     * @param ext_args 传递给 user_data_type 构造函数的参数
     * @note 仅当用户数据策略支持扩展（`supports_user_data == true`）时此构造函数可用。
     *       编译期检查 user_data_type 是否可从给定的参数类型构造。
     */
    template <typename... ExtArgs,
              typename UserP = UserTcbDataPolicy,
              typename       = std::enable_if_t<UserP::supports_user_data>>
    explicit Tcb(void (*entry_func)(), priority_type prio, task_id_type task_id, ExtArgs&&... ext_args)
        : st_tcb_type{entry_func, prio, task_id}
        , user_data_type(std::forward<ExtArgs>(ext_args)...) {
        static_assert(std::is_constructible_v<user_data_type, ExtArgs...>,
                      "user_data_type must be constructible from the provided arguments");
    }

    /// 编译期常量：是否支持平台上下文
    static constexpr bool supports_platform_context = platform_context::supports_platform_context;
    /// 编译期常量：是否支持用户数据扩展
    static constexpr bool supports_user_data = user_tcb_data::supports_user_data;

    // 禁止拷贝和移动（任务不应被复制或转移所有权）
    Tcb(const Tcb&)            = delete;
    Tcb& operator=(const Tcb&) = delete;
    Tcb(Tcb&&)                 = delete;
    Tcb& operator=(Tcb&&)      = delete;
};

} // namespace strat_os::kernel

#endif // STRATOS_KERNEL_TCB_HPP