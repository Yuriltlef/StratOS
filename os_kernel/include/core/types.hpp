/**
 * @file types.hpp
 * @author StratOS Team
 * @brief 内核类型策略接口与适配器（优先级、节拍计数、任务ID、任务状态）
 * @version 1.1.0
 * @date 2026-04-05
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了内核配置策略的抽象接口，采用静态策略模式。
 * 策略类必须提供以下嵌套类型：
 * - `priority_type`：优先级类型（无符号整数）
 * - `tick_type`：系统节拍计数类型（无符号整数）
 * - `task_id_type`：任务标识符类型（无符号整数）
 * - `task_state_size_type`：任务状态枚举的底层存储类型（无符号整数）
 * - `task_state_type`：任务状态枚举类，必须包含 `Ready`, `Running`, `Blocked`, `Suspended`, `Terminated` 五个枚举项
 *
 * 这些类型将被内核各组件（任务调度器、延时管理、任务标识、状态机）统一使用。
 *
 * 该设计通过类型萃取在编译期验证策略的完整性，包括：
 * - 类型是否存在
 * - 是否为无符号整数
 * - 是否满足最小宽度要求（可配置）
 * - 枚举成员是否存在（名称检查）
 *
 * 最终通过适配器模板 `KernelTypes` 提供统一的静态类型别名，供内核其余部分使用。
 *
 * 主要特性：
 * - 策略类只需定义五个类型，零运行时开销
 * - 编译期验证策略是否满足要求，错误信息清晰
 * - 便于移植到不同平台（例如 8 位机可使用 `uint8_t` 优先级，32 位机可使用 `uint32_t`）
 *
 * @note 典型的 `priority_type` 为无符号整数（如 `uint8_t`），数值越小优先级越低或越高（由调度器策略决定）。
 *       `tick_type` 通常为 `uint32_t` 或 `uint64_t`，用于表示系统运行以来的节拍数。
 *       `task_id_type` 通常为 `uint16_t` 或 `uint32_t`，用于唯一标识任务。
 *       `task_state_size_type` 通常为 `uint8_t`，与 `task_state_type` 的底层类型一致。
 *       `task_state_type` 必须包含 `Ready`, `Running`, `Blocked`, `Suspended`, `Terminated` 五个枚举项，
 *       且枚举值顺序可以任意（内核只依赖名称，不依赖具体数值）。
 *
 * @warning 策略类必须确保这些类型在目标平台上具有合理的取值范围，否则可能导致溢出或标识重复。
 *         自定义任务状态枚举时，名称必须完全匹配（大小写敏感），否则编译失败。
 */
#pragma once

#ifndef STRATOS_KERNEL_TYPES_HPP
#define STRATOS_KERNEL_TYPES_HPP

#include <cstdint>     // for std::uint32_t, std::uint16_t, std::uint8_t
#include <type_traits> // for std::false_type, std::true_type, std::conjunction

namespace strat_os::kernel::traits
{

// ----------------------------------------------------------------------------
// 基础类型存在性检测
// ----------------------------------------------------------------------------

/**
 * @brief 检测类型 T 是否包含嵌套类型 priority_type
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct has_priority_type : std::false_type {};
template <typename T>
struct has_priority_type<T, std::void_t<typename T::priority_type>> : std::true_type {};
template <typename T>
static constexpr bool has_priority_type_v = has_priority_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 tick_type
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct has_tick_type : std::false_type {};
template <typename T>
struct has_tick_type<T, std::void_t<typename T::tick_type>> : std::true_type {};
template <typename T>
static constexpr bool has_tick_type_v = has_tick_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 task_id_type
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct has_task_id_type : std::false_type {};
template <typename T>
struct has_task_id_type<T, std::void_t<typename T::task_id_type>> : std::true_type {};
template <typename T>
static constexpr bool has_task_id_type_v = has_task_id_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 task_state_size_type
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct has_task_state_size_type : std::false_type {};
template <typename T>
struct has_task_state_size_type<T, std::void_t<typename T::task_state_size_type>> : std::true_type {};
template <typename T>
static constexpr bool has_task_state_size_type_v = has_task_state_size_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 task_state_type
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct has_task_state_type : std::false_type {};
template <typename T>
struct has_task_state_type<T, std::void_t<typename T::task_state_type>> : std::true_type {};
template <typename T>
static constexpr bool has_task_state_type_v = has_task_state_type<T>::value;

// ----------------------------------------------------------------------------
// 类型合法性检测（无符号整数 + 最小宽度）
// ----------------------------------------------------------------------------

/**
 * @brief 检测 priority_type 是否为无符号整数且至少 8 位宽
 * @tparam T 待检测的策略类型（必须已包含 priority_type）
 */
template <typename T, typename = void>
struct is_valid_priority_type : std::false_type {};
template <typename T>
struct is_valid_priority_type<T, std::void_t<typename T::priority_type>>
    : std::conjunction<std::is_unsigned<typename T::priority_type>,
                       std::bool_constant<sizeof(typename T::priority_type) >= sizeof(std::uint8_t)>> {};
template <typename T>
static constexpr bool is_valid_priority_type_v = is_valid_priority_type<T>::value;

/**
 * @brief 检测 tick_type 是否为无符号整数且至少 32 位宽
 * @tparam T 待检测的策略类型（必须已包含 tick_type）
 */
template <typename T, typename = void>
struct is_valid_tick_type : std::false_type {};
template <typename T>
struct is_valid_tick_type<T, std::void_t<typename T::tick_type>>
    : std::conjunction<std::is_unsigned<typename T::tick_type>,
                       std::bool_constant<sizeof(typename T::tick_type) >= sizeof(std::uint32_t)>> {};
template <typename T>
static constexpr bool is_valid_tick_type_v = is_valid_tick_type<T>::value;

/**
 * @brief 检测 task_id_type 是否为无符号整数且至少 16 位宽
 * @tparam T 待检测的策略类型（必须已包含 task_id_type）
 */
template <typename T, typename = void>
struct is_valid_task_id_type : std::false_type {};
template <typename T>
struct is_valid_task_id_type<T, std::void_t<typename T::task_id_type>>
    : std::conjunction<std::is_unsigned<typename T::task_id_type>,
                       std::bool_constant<sizeof(typename T::task_id_type) >= sizeof(std::uint16_t)>> {};
template <typename T>
static constexpr bool is_valid_task_id_type_v = is_valid_task_id_type<T>::value;

/**
 * @brief 检测 task_state_size_type 是否为无符号整数且至少 8 位宽
 * @tparam T 待检测的策略类型（必须已包含 task_state_size_type）
 */
template <typename T, typename = void>
struct is_valid_task_state_size_type : std::false_type {};
template <typename T>
struct is_valid_task_state_size_type<T, std::void_t<typename T::task_state_size_type>>
    : std::conjunction<std::is_unsigned<typename T::task_state_size_type>,
                       std::bool_constant<sizeof(typename T::task_state_size_type) >= sizeof(std::uint8_t)>> {};
template <typename T>
static constexpr bool is_valid_task_state_size_type_v = is_valid_task_state_size_type<T>::value;

// ----------------------------------------------------------------------------
// 任务状态枚举成员存在性检测（名称检查）
// ----------------------------------------------------------------------------

/**
 * @brief 检测枚举类型 E 是否包含 Ready 枚举项
 * @tparam E 枚举类型
 */
template <typename E, typename = void>
struct has_ready_enumerator : std::false_type {};
template <typename E>
struct has_ready_enumerator<E, std::void_t<decltype(E::Ready)>> : std::true_type {};

/**
 * @brief 检测枚举类型 E 是否包含 Running 枚举项
 */
template <typename E, typename = void>
struct has_running_enumerator : std::false_type {};
template <typename E>
struct has_running_enumerator<E, std::void_t<decltype(E::Running)>> : std::true_type {};

/**
 * @brief 检测枚举类型 E 是否包含 Blocked 枚举项
 */
template <typename E, typename = void>
struct has_blocked_enumerator : std::false_type {};
template <typename E>
struct has_blocked_enumerator<E, std::void_t<decltype(E::Blocked)>> : std::true_type {};

/**
 * @brief 检测枚举类型 E 是否包含 Suspended 枚举项
 */
template <typename E, typename = void>
struct has_suspended_enumerator : std::false_type {};
template <typename E>
struct has_suspended_enumerator<E, std::void_t<decltype(E::Suspended)>> : std::true_type {};

/**
 * @brief 检测枚举类型 E 是否包含 Terminated 枚举项
 */
template <typename E, typename = void>
struct has_terminated_enumerator : std::false_type {};
template <typename E>
struct has_terminated_enumerator<E, std::void_t<decltype(E::Terminated)>> : std::true_type {};

/**
 * @brief 组合检测枚举类型是否包含所有五个必需枚举项
 * @tparam E 枚举类型
 */
template <typename E>
struct has_required_task_state_enumerators : std::conjunction<has_ready_enumerator<E>,
                                                              has_running_enumerator<E>,
                                                              has_blocked_enumerator<E>,
                                                              has_suspended_enumerator<E>,
                                                              has_terminated_enumerator<E>> {};
template <typename E>
static constexpr bool has_required_task_state_enumerators_v = has_required_task_state_enumerators<E>::value;

// ----------------------------------------------------------------------------
// task_state_type 综合合法性检测
// ----------------------------------------------------------------------------

/**
 * @brief 检测 task_state_type 是否合法
 * @details 要求：
 *          - 是一个枚举类
 *          - 其底层存储大小与 task_state_size_type 一致
 *          - 包含 Ready, Running, Blocked, Suspended, Terminated 五个枚举项
 * @tparam T 策略类型（必须已包含 task_state_type 和 task_state_size_type）
 */
template <typename T, typename = void, typename = void>
struct is_valid_task_state_type : std::false_type {};
template <typename T>
struct is_valid_task_state_type<T,
                                std::void_t<typename T::task_state_type>,
                                std::void_t<typename T::task_state_size_type>>
    : std::conjunction<
          std::is_enum<typename T::task_state_type>,
          std::bool_constant<sizeof(typename T::task_state_type) == sizeof(typename T::task_state_size_type)>,
          has_required_task_state_enumerators<typename T::task_state_type>> {};
template <typename T>
static constexpr bool is_valid_task_state_type_v = is_valid_task_state_type<T>::value;

// ----------------------------------------------------------------------------
// 组合检测：完整的 KernelConfigPolicy 有效性
// ----------------------------------------------------------------------------

/**
 * @brief 组合检测，判断类型 T 是否为有效的 KernelConfigPolicy 策略
 * @tparam T 待检测的策略类型
 *
 * 要求 T 必须定义所有必需的嵌套类型，并满足各自的合法性约束。
 *
 * @note 如果策略类不满足要求，`is_valid_kernel_config_policy_v<T>` 为 false，
 *       适配器模板将使用 `std::enable_if` 禁止实例化，并触发静态断言提供错误信息。
 */
template <typename T>
struct is_valid_kernel_config_policy : std::conjunction<has_priority_type<T>,
                                                        has_tick_type<T>,
                                                        has_task_id_type<T>,
                                                        has_task_state_size_type<T>,
                                                        has_task_state_type<T>,
                                                        is_valid_priority_type<T>,
                                                        is_valid_tick_type<T>,
                                                        is_valid_task_id_type<T>,
                                                        is_valid_task_state_size_type<T>,
                                                        is_valid_task_state_type<T>> {};
template <typename T>
static constexpr bool is_valid_kernel_config_policy_v = is_valid_kernel_config_policy<T>::value;

} // namespace strat_os::kernel::traits

namespace strat_os::kernel
{

/**
 * @brief 内核类型适配器模板
 * @tparam KernelConfigPolicy 具体的策略类，必须满足 KernelConfigPolicy 接口（提供所有必需嵌套类型）
 *
 * 该类将策略类包装为统一的内核类型集，并进行编译期验证。
 * 所有类型别名直接转发到策略类，不引入任何额外开销。
 *
 * @par 使用示例
 * @code
 * // 使用默认配置
 * using MyKernelTypes = KernelTypes<>;
 *
 * // 使用自定义配置
 * struct MyConfig {
 *     using priority_type = uint16_t;
 *     using tick_type = uint64_t;
 *     using task_id_type = uint32_t;
 *     using task_state_size_type = uint8_t;
 *     enum class task_state_type : task_state_size_type {
 *         Ready, Running, Blocked, Suspended, Terminated, MyCustomState
 *     };
 * };
 * using MyKernelTypes = KernelTypes<MyConfig>;
 *
 * // 使用类型别名
 * MyKernelTypes::priority_type prio = 10;
 * MyKernelTypes::tick_type ticks = 1000;
 * MyKernelTypes::task_id_type id = 1;
 * MyKernelTypes::task_state state = MyKernelTypes::task_state::Ready;
 * @endcode
 *
 * @note 策略类通常由用户或板级配置提供，见 `os_kernel/config/kernel_config.hpp` 中的默认实现。
 * @warning 策略类中的类型必须满足内核各组件对取值范围、算术运算等隐式要求，
 *          否则可能导致编译错误或运行时未定义行为。
 */
template <typename KernelConfigPolicy,
          typename = std::enable_if_t<traits::is_valid_kernel_config_policy_v<KernelConfigPolicy>>>
struct KernelTypes {
    /// 策略类别名
    using Policy = KernelConfigPolicy;

    // ----- 细粒度静态断言，提供清晰的错误信息 -----
    static_assert(traits::has_priority_type_v<Policy>, "KernelConfigPolicy must define 'priority_type'");
    static_assert(traits::has_tick_type_v<Policy>, "KernelConfigPolicy must define 'tick_type'");
    static_assert(traits::has_task_id_type_v<Policy>, "KernelConfigPolicy must define 'task_id_type'");
    static_assert(traits::has_task_state_type_v<Policy>, "KernelConfigPolicy must define 'task_state_type'");
    static_assert(traits::has_task_state_size_type_v<Policy>, "KernelConfigPolicy must define 'task_state_size_type'");

    static_assert(traits::is_valid_priority_type_v<Policy>,
                  "Policy::priority_type must be an unsigned integer type and have at least 8 bits");
    static_assert(
        traits::is_valid_tick_type_v<Policy>,
        "Policy::tick_type must be an unsigned integer type and have at least 32 bits to avoid rapid overflow");
    static_assert(traits::is_valid_task_id_type_v<Policy>,
                  "Policy::task_id_type must be an unsigned integer type and have at least 16 bits to support a "
                  "reasonable number of tasks");
    static_assert(traits::is_valid_task_state_size_type_v<Policy>,
                  "Policy::task_state_size_type must be an unsigned integer type and have at least 8 bits");
    static_assert(traits::is_valid_task_state_type_v<Policy>,
                  "task_state_type must be an enum, its size must match task_state_size_type, "
                  "and it must contain enumerators: Ready, Running, Blocked, Suspended, Terminated. "
                  "Please check your enum definition.");

    /// 任务优先级类型（无符号整数，至少 8 位）
    using priority_type = typename Policy::priority_type;
    /// 系统节拍计数类型（无符号整数，至少 32 位）
    using tick_type = typename Policy::tick_type;
    /// 任务 ID 类型（无符号整数，至少 16 位）
    using task_id_type = typename Policy::task_id_type;
    /// 任务状态枚举的底层存储类型（无符号整数，至少 8 位）
    using task_state_size_type = typename Policy::task_state_size_type;
    /// 任务状态枚举类（必须包含 Ready, Running, Blocked, Suspended, Terminated）
    using task_state_type = typename Policy::task_state_type;
    /// 任务状态枚举类别名（便于使用）
    using task_state = task_state_type;
};

/**
 * @brief 空基类，用于拓展。
 */
struct EmptyBase {};

} // namespace strat_os::kernel

namespace strat_os::kernel::traits
{
// ----------------------------------------------------------------------------
// 用户数据类型检测
// ----------------------------------------------------------------------------

/**
 * @brief 检测类型 T 是否包含嵌套类型 user_data_type
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct has_user_data_type : std::false_type {};
template <typename T>
struct has_user_data_type<T, std::void_t<typename T::user_data_type>> : std::true_type {};
template <typename T>
static constexpr bool has_user_data_type_v = has_user_data_type<T>::value;

/**
 * @brief 检测类型 T 是否包含静态常量 supports_user_data
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct has_supports_user_data : std::false_type {};
template <typename T>
struct has_supports_user_data<T, std::void_t<decltype(T::supports_user_data)>> : std::true_type {};
template <typename T>
static constexpr bool has_supports_user_data_v = has_supports_user_data<T>::value;

/**
 * @brief 检测 supports_user_data 是否为 const bool 类型
 * @tparam T 待检测的策略类型
 */
template <typename T, typename = void>
struct is_valid_supports_user_data : std::false_type {};
template <typename T>
struct is_valid_supports_user_data<T, std::void_t<decltype(T::supports_user_data)>>
    : std::is_same<decltype(T::supports_user_data), const bool> {};
template <typename T>
static constexpr bool is_valid_supports_user_data_v = is_valid_supports_user_data<T>::value;

/**
 * @brief 组合检测，判断类型 T 是否为合法用户拓展tcb策略
 * @tparam T 待检测的策略类型
 *
 * @note 要求：
 * - 必须定义 supports_user_data 常量（const bool）
 * - 必须定义 user_data_type 类型，
 * - supports_user_data 为 false，user_data_type 为空类
 *
 */
template <typename T>
static constexpr bool is_valid_user_data_policy_v =
    has_supports_user_data_v<T> && is_valid_supports_user_data_v<T> &&
    (!T::supports_user_data && std::is_empty_v<T> || T::supports_user_data);

} // namespace strat_os::kernel::traits

namespace strat_os::kernel
{
/**
 * @brief 用户扩展TCB类型适配器模板
 * @tparam UserTcbDataPolicy 具体的策略类，必须满足 UserTcbDataPolicy 接口（提供所有必需嵌套类型）
 *
 * 该类将策略类包装为统一的内核类型集，并进行编译期验证。
 * 所有类型别名直接转发到策略类，不引入任何额外开销。
 * @note 当supports_user_data为false，user_data_type必须为空类。
 */
template <typename UserTcbDataPolicy,
          typename = std::enable_if_t<traits::is_valid_user_data_policy_v<UserTcbDataPolicy>>>
struct UserTcbData {
    using Policy = UserTcbDataPolicy;
    static_assert(traits::has_user_data_type_v<Policy>, "UserTcbDataPolicy must define 'user_data_type'");

    /// 是否支持用户数据（编译期常量）
    static constexpr bool supports_user_data = Policy::supports_user_data;
    /// 用户拓展类型别名
    using user_data_type = typename UserTcbDataPolicy::user_data_type;
};

} // namespace strat_os::kernel

#endif // STRATOS_KERNEL_TYPES_HPP