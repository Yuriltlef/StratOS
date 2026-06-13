/**
 * @file tcb.hpp
 * @author StratOS Team
 * @brief 内置用户 TCB 数据扩展策略（空实现）
 * @version 1.0.0
 * @date 2026-04-07
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件提供了 StratOS 内核中用户 TCB 数据扩展策略的默认实现，
 * 属于官方内置策略（builtin）。该策略声明不支持用户数据扩展，
 * 即 `supports_user_data` 为 false，并将 `user_data_type` 定义为空类型。
 *
 * 当用户不需要为任务控制块（TCB）添加自定义字段时，可以使用此策略。
 * 空类型通过空基类优化（EBO）不会增加 TCB 的内存占用。
 *
 * 该策略满足 `strat_os::kernel::UserTcbData` 适配器的接口要求，
 * 可直接作为模板参数使用。
 *
 * @note 如果用户需要扩展 TCB（例如添加任务名称、截止时间等），
 *       应自定义策略类，定义 `supports_user_data = true` 并提供
 *       具体的 `user_data_type` 结构体。
 */
#pragma once

#ifndef STRATOS_POLICY_KERNEL_TCB_HPP
#define STRATOS_POLICY_KERNEL_TCB_HPP

#include "os_kernel/include/core/types.hpp"

namespace strat_os::kernel::policy::builtin
{

/**
 * @brief 内置用户 TCB 数据策略（无扩展）
 *
 * 该策略表示用户不需要为 TCB 添加任何自定义数据。
 * 它定义 `supports_user_data = false`，内核适配器将据此禁用
 * 需要用户数据参数的构造函数，且 TCB 不会包含额外的用户数据成员。
 */
struct UserTcbDataPolicy {
    /// 表示不支持用户数据扩展
    static constexpr bool supports_user_data = false;

    /// 用户数据类型定义为空基类（利用空基类优化）
    using user_data_type = strat_os::kernel::EmptyBase;
};

} // namespace strat_os::kernel::policy::builtin

#endif // STRATOS_POLICY_KERNEL_TCB_HPP