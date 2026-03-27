/**
 * @file basic_error.hpp
 * @author YT_Minro (yurilt15312@outlook.com)
 * @brief 基础错误处理定义：错误码、错误上下文、错误处理器合法性检查及基础处理器模板
 * @version 1.0.0
 * @date 2026-03-14
 *
 * @copyright Copyright (c) 2026
 *
 * @details
 * 提供嵌入式高安全环境下的基础错误处理组件：
 * - 错误码枚举（ErrorCode）：涵盖常见容器错误及系统服务错误，预留用户扩展空间。
 * - 精简错误上下文（ErrorContext）：仅包含错误码和一个额外数据字段，最小化开销。
 * - 错误处理器合法性检查 trait（is_valid_error_handler）：确保自定义处理器满足
 *   静态要求（包含 id、is_fatal 类型、可调用且接受 const ErrorContext&）。
 * - 基础错误处理器模板（ErrorHandler）：通过模板参数指定致命性（IsFatal）和标识（Id），
 *   提供默认行为（致命错误死循环，非致命错误忽略）。
 * - 便捷类型别名（FatalErrorHandler / NonFatalErrorHandler）：快速生成致命/非致命处理器。
 *
 * 该组件与 StaticArray、RingBuffer 等容器无缝集成，实现编译期可配置的错误处理策略。
 * 所有定义均无动态内存分配，无异常，符合 MISRA 及高安全嵌入式设计原则。
 *
 * @par 使用示例
 * @code
 * #include "mu_sstl/errors/basic_error.hpp"
 *
 * // 自定义致命错误处理器（必须永不返回）
 * struct MyFatalHandler : mu_sstl::FatalErrorHandler<42> {
 *     void operator()(const mu_sstl::ErrorContext& ctx) const override {
 *         // 记录错误码 ctx.code 和附加数据 ctx.extra
 *         // 复位系统
 *         NVIC_SystemReset();
 *     }
 * };
 *
 * // 自定义非致命错误处理器（可返回）
 * struct MyNonFatalHandler : mu_sstl::NonFatalErrorHandler<43> {
 *     void operator()(const mu_sstl::ErrorContext& ctx) const override {
 *         // 记录日志，然后正常返回
 *         log_error(ctx.code, ctx.extra);
 *     }
 * };
 *
 * // 在容器中使用（容器要求处理器为致命类型）
 * using SafeArray = mu_sstl::StaticArray<SomePolicy, MyFatalHandler>;
 * @endcode
 *
 * @note 错误处理器的 is_fatal 类型用于编译期区分致命/非致命行为。
 *       容器内部应使用致命处理器（永不返回），非致命处理器可用于系统服务层。
 * @warning 若自定义处理器不满足 is_valid_error_handler 的要求，容器实例化将失败，
 *          并产生静态断言提示。
 * @warning 错误处理器 ID 0~31 预留给操作系统（内核），用户自定义 ID 建议 ≥32。
 */

#pragma once

#ifndef MU_SSTL_BASIC_ERROR_HPP
#define MU_SSTL_BASIC_ERROR_HPP

#include <cstddef>     // for std::size_t
#include <cstdint>     // for uint32_t
#include <type_traits> // for std::true_type, std::false_type, etc.

namespace mu_sstl
{

/// @brief 预留给操作系统的错误处理器数量（ID 0~31 为内核保留）
constexpr std::size_t os_reserved_error_handle_count{32};

/**
 * @brief 错误码枚举
 *
 * 涵盖常见容器错误及系统级错误，用户可自行扩展。
 * 枚举值按功能分类，预留用户自定义起始位置。
 */
enum class ErrorCode : std::uint32_t {
    // 通用错误 (0x0000 - 0x00FF)
    None = 0x0000, ///< 无错误（通常不使用）
    Unknown,       ///< 未知错误

    // 容器错误 (0x0100 - 0x01FF)
    OutOfBounds = 0x0100, ///< 索引越界
    Full,                 ///< 容器已满
    Empty,                ///< 容器为空
    InvalidArgument,      ///< 无效参数
    AllocationFailed,     ///< 对象分配失败
    NotEnoughSpace,       ///< 空间不足
    OutOfMemory,          ///< 内存耗尽（动态分配失败）

    // 系统服务错误 (0x0200 - 0x02FF)
    Timeout = 0x0200, ///< 操作超时
    ResourceBusy,     ///< 资源忙
    InvalidHandle,    ///< 无效句柄
    NotSupported,     ///< 不支持的操作

    // 用户自定义错误起始位置 (0x8000)
    UserBase = 0x8000
};

/**
 * @brief 最精简错误上下文
 *
 * 仅包含错误码和一个额外的数据字段，用于传递简单的数值信息（如索引、大小等）。
 * 文件名、行号等调试信息由用户在处理函数中自行获取（例如通过调用处宏）。
 */
struct ErrorContext {
    ErrorCode code;      ///< 错误码
    std::uint32_t extra; ///< 额外数据（如资源标识符、错误相关数值）

    /**
     * @brief 构造一个错误上下文
     * @param ec   错误码
     * @param ex   额外数据（默认0）
     */
    explicit constexpr ErrorContext(ErrorCode ec, std::uint32_t ex = 0) noexcept
        : code(ec)
        , extra(ex) {}
};

// -------------------- 错误处理器合法性检查 --------------------

/// @brief 检测类型 T 是否包含静态常量 id 成员
template <typename T, typename = void>
struct has_id : std::false_type {};

template <typename T>
struct has_id<T, std::void_t<decltype(T::id)>> : std::true_type {};

template <typename T>
inline constexpr bool has_id_v = has_id<T>::value;

/// @brief 检测类型 T 是否包含内嵌类型 is_fatal
template <typename T, typename = void>
struct has_is_fatal_type : std::false_type {};

template <typename T>
struct has_is_fatal_type<T, std::void_t<typename T::is_fatal>> : std::true_type {};

template <typename T>
inline constexpr bool has_is_fatal_type_v = has_is_fatal_type<T>::value;

/// @brief 检查 is_fatal 是否为 std::true_type 或 std::false_type
template <typename T, bool = has_is_fatal_type_v<T>>
struct is_is_fatal_valid_impl : std::false_type {};

template <typename T>
struct is_is_fatal_valid_impl<T, true>
    : std::integral_constant<bool,
                             std::is_same_v<typename T::is_fatal, std::true_type> ||
                                 std::is_same_v<typename T::is_fatal, std::false_type>> {};

template <typename T>
struct is_is_fatal_valid : is_is_fatal_valid_impl<T> {};

template <typename T>
inline constexpr bool is_is_fatal_valid_v = is_is_fatal_valid<T>::value;

/// @brief 检测是否可以用 const ErrorContext& 调用 operator()
template <typename T, typename = void>
struct has_const_call_with_context : std::false_type {};

template <typename T>
struct has_const_call_with_context<T,
                                   std::void_t<decltype(std::declval<const T&>()(std::declval<const ErrorContext&>()))>>
    : std::true_type {};

template <typename T>
inline constexpr bool has_const_call_with_context_v = has_const_call_with_context<T>::value;

/// @brief 获取 id 的值（假设存在）
template <typename T>
constexpr std::uint32_t get_id_value() {
    static_assert(has_id_v<T>, "T must have a static constexpr id member");
    return T::id;
}

/// @brief 检查非致命处理器的 id 是否有效（id >= 32）
template <typename T, bool IsFatal>
struct id_valid_for_nonfatal_impl : std::true_type {};

template <typename T>
struct id_valid_for_nonfatal_impl<T, false>
    : std::integral_constant<bool, (get_id_value<T>() >= os_reserved_error_handle_count)> {};

template <typename T>
struct id_valid_for_nonfatal : id_valid_for_nonfatal_impl<T, std::is_same_v<typename T::is_fatal, std::true_type>> {};

template <typename T>
inline constexpr bool id_valid_for_nonfatal_v = id_valid_for_nonfatal<T>::value;

/**
 * @brief 综合检查：判断类型 T 是否为合法的错误处理器
 *
 * 合法处理器必须满足：
 * - 包含内嵌类型 is_fatal，且为 std::true_type 或 std::false_type
 * - 包含静态常量 id
 * - 若 is_fatal == false，则 id 必须 ≥ os_reserved_error_handle_count
 * - 提供 const 限定的 operator()，接受 const ErrorContext& 参数
 */
template <typename T>
struct is_valid_error_handler
    : std::integral_constant<bool,
                             has_is_fatal_type_v<T> && is_is_fatal_valid_v<T> && has_id_v<T> &&
                                 id_valid_for_nonfatal_v<T> && has_const_call_with_context_v<T>> {};

template <typename T>
inline constexpr bool is_valid_error_handler_v = is_valid_error_handler<T>::value;

// -------------------- 基础错误处理器模板 --------------------

/**
 * @brief 基础错误处理器模板
 *
 * @tparam IsFatal 致命性标志，必须为 std::true_type 或 std::false_type
 * @tparam Id      处理器标识（编译期常量）
 *
 * 提供默认的 operator() 实现：
 * - 若 IsFatal 为 true，进入死循环（永不返回）
 * - 若 IsFatal 为 false，空操作（可正常返回）
 *
 * 用户可通过继承并覆盖 operator() 自定义处理逻辑。
 */
template <typename IsFatal,
          std::uint32_t Id,
          typename =
              std::enable_if_t<std::is_same_v<IsFatal, std::true_type> || std::is_same_v<IsFatal, std::false_type>>>
struct ErrorHandler {
    static constexpr std::uint32_t id = Id; ///< 错误处理器标识（0~31 预留给操作系统）
    using is_fatal                    = IsFatal;
    ErrorHandler()                    = default;
    /**
     * @brief 处理错误
     * @param ctx 错误上下文
     *
     * 默认行为：
     * - 致命错误：死循环（永不返回）
     * - 非致命错误：忽略参数，直接返回
     */
    [[noreturn]] void operator()([[maybe_unused]] const ErrorContext& ctx) const {
        if constexpr (IsFatal::value) {
            // 致命错误：死循环（永不返回）
            while (true) {}
        } else {
            // 非致命错误：默认空操作
        }
    }
};

/// @brief 致命错误处理器别名（永不返回）
template <std::uint32_t Id>
using FatalErrorHandler = ErrorHandler<std::true_type, Id>;

/// @brief 非致命错误处理器别名（可返回）
template <std::uint32_t Id>
using NonFatalErrorHandler = ErrorHandler<std::false_type, Id>;

} // namespace mu_sstl

#endif // MU_SSTL_BASIC_ERROR_HPP