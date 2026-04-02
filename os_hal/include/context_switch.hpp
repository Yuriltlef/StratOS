/**
 * @file context_switch.hpp
 * @author StratOS Team
 * @brief 上下文切换策略接口与适配器
 * @version 1.0.0
 * @date 2026-03-28
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了硬件抽象层（HAL）的上下文切换抽象接口，采用静态策略模式。
 * 它将与任务切换相关的硬件操作（如触发 PendSV、栈指针操作、特权级切换、
 * 内存屏障等）封装为策略类，并通过类型萃取在编译期验证策略的完整性，
 * 最终通过适配器模板提供统一的静态接口供 RTOS 内核使用。
 *
 * 该设计使得内核代码与平台完全解耦，同时保持零开销抽象：所有方法均为
 * 内联且 noexcept，在编译期直接展开为底层硬件指令。
 *
 * 主要组件：
 * - traits 命名空间：提供一系列类型萃取，用于检测策略类是否满足接口约定。
 * - ContextSwitch 适配器模板：接收一个策略类，进行编译期验证，并转发所有操作。
 * - 可选功能检测：如获取当前异常号、多核支持（核心 ID、核间中断）等，
 *   通过 SFINAE 自动提供相应重载，避免代码膨胀。
 *
 * @note 策略类必须提供 word 类型别名（无符号整数，大小等于指针），
 *       以及所有必需静态方法（trigger_pendsv、set_psp、get_psp、set_msp、
 *       get_msp、switch_to_unprivileged、switch_to_privileged、dmb、dsb、isb）。
 *       可选方法（get_current_exception、core_id、send_ipi）可根据硬件能力选择性实现。
 *
 * @warning 所有方法均假设在特权模式下调用，且调用前已确保当前上下文合法。
 */
#pragma once

#ifndef STRATOS_HAL_CONTEXT_SWITCH_HPP
#define STRATOS_HAL_CONTEXT_SWITCH_HPP

#include <type_traits> // for std::false_type, std::true_type, etc.
#include <utility>     // for std::declval

namespace strat_os::hal::traits
{
/**
 * @brief 检测类型 T 是否包含嵌套类型 word
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_word_type : std::false_type {};
template <typename T>
struct has_word_type<T, std::void_t<typename T::word>> : std::true_type {};
template <typename T>
static constexpr bool has_word_type_v = has_word_type<T>::value;

/**
 * @brief 检测 word 类型是否为有效的栈指针指向的地址类型(本身就是地址数值)
 * @tparam T word 类型
 *
 * 要求 word 为无符号整数类型，且大小等于指针大小（通常为 32 位）。
 */
template <typename T>
struct is_valid_word_type
    : std::conjunction<std::is_unsigned<T>, std::integral_constant<bool, sizeof(T) == sizeof(nullptr)>> {};
template <typename T>
static constexpr bool is_valid_word_type_v = is_valid_word_type<T>::value;

/**
 * @brief 检测静态方法 trigger_pendsv()
 */
template <typename T, typename = void>
struct has_trigger_pendsv_method : std::false_type {};
template <typename T>
struct has_trigger_pendsv_method<T, std::void_t<decltype(T::trigger_pendsv())>> : std::true_type {};
template <typename T>
static constexpr bool has_trigger_pendsv_method_v = has_trigger_pendsv_method<T>::value;

/**
 * @brief 检测静态方法 set_psp(word)
 */
template <typename T, typename = void>
struct has_set_psp_method : std::false_type {};
template <typename T>
struct has_set_psp_method<T, std::void_t<decltype(T::set_psp(std::declval<typename T::word>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_set_psp_method_v = has_set_psp_method<T>::value;

/**
 * @brief 检测静态方法 get_psp() -> word
 */
template <typename T, typename = void>
struct has_get_psp_method : std::false_type {};
template <typename T>
struct has_get_psp_method<T, std::void_t<decltype(T::get_psp())>> : std::true_type {};
template <typename T>
static constexpr bool has_get_psp_method_v = has_get_psp_method<T>::value;

/**
 * @brief 检测类型 T 的 get_psp() 返回类型是否为 word
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_correct_get_psp_return_type : std::is_same<decltype(T::get_psp()), typename T::word> {};
template <typename T>
static constexpr bool is_correct_get_psp_return_type_v = is_correct_get_psp_return_type<T>::value;

/**
 * @brief 检测静态方法 set_msp(word)
 */
template <typename T, typename = void>
struct has_set_msp_method : std::false_type {};
template <typename T>
struct has_set_msp_method<T, std::void_t<decltype(T::set_msp(std::declval<typename T::word>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_set_msp_method_v = has_set_msp_method<T>::value;

/**
 * @brief 检测静态方法 get_msp() -> word
 */
template <typename T, typename = void>
struct has_get_msp_method : std::false_type {};
template <typename T>
struct has_get_msp_method<T, std::void_t<decltype(T::get_msp())>> : std::true_type {};
template <typename T>
static constexpr bool has_get_msp_method_v = has_get_msp_method<T>::value;

/**
 * @brief 检测类型 T 的 get_msp() 返回类型是否为 word
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_correct_get_msp_return_type : std::is_same<decltype(T::get_msp()), typename T::word> {};
template <typename T>
static constexpr bool is_correct_get_msp_return_type_v = is_correct_get_msp_return_type<T>::value;

/**
 * @brief 检测静态方法 switch_to_unprivileged()
 */
template <typename T, typename = void>
struct has_switch_to_unprivileged_method : std::false_type {};
template <typename T>
struct has_switch_to_unprivileged_method<T, std::void_t<decltype(T::switch_to_unprivileged())>> : std::true_type {};
template <typename T>
static constexpr bool has_switch_to_unprivileged_method_v = has_switch_to_unprivileged_method<T>::value;

/**
 * @brief 检测静态方法 switch_to_privileged()
 */
template <typename T, typename = void>
struct has_switch_to_privileged_method : std::false_type {};
template <typename T>
struct has_switch_to_privileged_method<T, std::void_t<decltype(T::switch_to_privileged())>> : std::true_type {};
template <typename T>
static constexpr bool has_switch_to_privileged_method_v = has_switch_to_privileged_method<T>::value;

/**
 * @brief 检测静态方法 dmb()
 */
template <typename T, typename = void>
struct has_dmb_method : std::false_type {};
template <typename T>
struct has_dmb_method<T, std::void_t<decltype(T::dmb())>> : std::true_type {};
template <typename T>
static constexpr bool has_dmb_method_v = has_dmb_method<T>::value;

/**
 * @brief 检测静态方法 dsb()
 */
template <typename T, typename = void>
struct has_dsb_method : std::false_type {};
template <typename T>
struct has_dsb_method<T, std::void_t<decltype(T::dsb())>> : std::true_type {};
template <typename T>
static constexpr bool has_dsb_method_v = has_dsb_method<T>::value;

/**
 * @brief 检测静态方法 isb()
 */
template <typename T, typename = void>
struct has_isb_method : std::false_type {};
template <typename T>
struct has_isb_method<T, std::void_t<decltype(T::isb())>> : std::true_type {};
template <typename T>
static constexpr bool has_isb_method_v = has_isb_method<T>::value;

// 可选方法检测
/**
 * @brief 检测静态方法 get_current_exception()
 * @note 可选，用于获取当前异常号（非零表示在中断中）。
 */
template <typename T, typename = void>
struct has_get_current_exception_method : std::false_type {};
template <typename T>
struct has_get_current_exception_method<T, std::void_t<decltype(T::get_current_exception())>> : std::true_type {};
template <typename T>
static constexpr bool has_get_current_exception_method_v = has_get_current_exception_method<T>::value;

/**
 * @brief 检测静态方法 core_id()
 * @note 可选，用于多核环境获取当前 CPU 核心编号。
 */
template <typename T, typename = void>
struct has_core_id_method : std::false_type {};
template <typename T>
struct has_core_id_method<T, std::void_t<decltype(T::core_id())>> : std::true_type {};
template <typename T>
static constexpr bool has_core_id_method_v = has_core_id_method<T>::value;

/**
 * @brief 检测静态方法 send_ipi(word)
 * @note 可选，用于多核环境向指定核心发送核间中断（IPI）。
 */
template <typename T, typename = void>
struct has_send_ipi_method : std::false_type {};
template <typename T>
struct has_send_ipi_method<T, std::void_t<decltype(T::send_ipi(std::declval<typename T::word>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_send_ipi_method_v = has_send_ipi_method<T>::value;

/**
 * @brief 组合检测：判断类型 T 是否为有效的上下文切换策略
 * @tparam T 待检测的类型
 *
 * 要求 T 必须提供 word 类型别名（无符号整数，大小等于指针）以及所有必需静态方法。
 */
template <typename T>
struct is_valid_context_switch_policy : std::conjunction<has_word_type<T>,
                                                         is_valid_word_type<typename T::word>,
                                                         has_trigger_pendsv_method<T>,
                                                         has_set_psp_method<T>,
                                                         has_get_psp_method<T>,
                                                         has_set_msp_method<T>,
                                                         has_get_msp_method<T>,
                                                         has_switch_to_unprivileged_method<T>,
                                                         has_switch_to_privileged_method<T>,
                                                         has_dmb_method<T>,
                                                         has_dsb_method<T>,
                                                         has_isb_method<T>,
                                                         is_correct_get_psp_return_type<T>,
                                                         is_correct_get_msp_return_type<T>> {};
template <typename T>
static constexpr bool is_valid_context_switch_policy_v = is_valid_context_switch_policy<T>::value;

/**
 * @brief 组合检测：策略是否支持多核（同时提供 core_id 和 send_ipi）
 */
template <typename T>
struct has_multicore_support : std::conjunction<has_core_id_method<T>, has_send_ipi_method<T>> {};
template <typename T>
static constexpr bool has_multicore_support_v = has_multicore_support<T>::value;

} // namespace strat_os::hal::traits

namespace strat_os::hal
{

/**
 * @brief 上下文切换适配器模板
 * @tparam ContextSwitchPolicy 具体的策略类，必须满足上下文切换策略接口
 *
 * 该类将策略类包装为统一的静态接口，并进行编译期验证。
 * 所有方法均为内联且 noexcept，转发到策略类的对应静态方法。
 *
 * 若策略支持可选功能（如获取异常号、多核支持），则会通过 SFINAE 提供相应重载；
 * 否则这些方法在适配器中不可用，从而避免代码膨胀。
 *
 * 使用示例：
 * @code
 * // 假设 CortexM3ContextSwitch 已提供所有必需方法
 * using MyContextSwitch = ContextSwitch<CortexM3ContextSwitch>;
 *
 * // 触发任务切换
 * MyContextSwitch::trigger_pendsv();
 *
 * // 切换到用户模式
 * MyContextSwitch::switch_to_unprivileged();
 *
 * // 获取当前异常号（若策略支持）
 * if constexpr (MyContextSwitch::supports_multicore) {
 *     auto core = MyContextSwitch::core_id();
 * }
 * @endcode
 *
 * @note 策略类必须定义 word 类型别名（通常为 uint32_t），且所有方法必须为 static。
 * @warning 部分方法（如 switch_to_unprivileged）在调用后可能需要立即执行 ISB 指令，
 *          适配器已包含该屏障，但策略实现应确保正确性。
 */
template <typename ContextSwitchPolicy,
          typename = std::enable_if_t<traits::is_valid_context_switch_policy_v<ContextSwitchPolicy>>>
struct ContextSwitch {
    /// 策略类别名
    using Policy = ContextSwitchPolicy;

    // 细粒度静态断言，提供清晰的错误信息
    static_assert(traits::has_word_type_v<Policy>, "ContextSwitch policy must define 'word' type");
    static_assert(traits::is_valid_word_type_v<typename Policy::word>,
                  "word must be an unsigned integer of pointer size");
    static_assert(traits::has_trigger_pendsv_method_v<Policy>, "ContextSwitch policy must provide trigger_pendsv()");
    static_assert(traits::has_set_psp_method_v<Policy>, "ContextSwitch policy must provide set_psp(word)");
    static_assert(traits::has_get_psp_method_v<Policy>, "ContextSwitch policy must provide get_psp()");
    static_assert(traits::has_set_msp_method_v<Policy>, "ContextSwitch policy must provide set_msp(word)");
    static_assert(traits::has_get_msp_method_v<Policy>, "ContextSwitch policy must provide get_msp()");
    static_assert(traits::has_switch_to_unprivileged_method_v<Policy>,
                  "ContextSwitch policy must provide switch_to_unprivileged()");
    static_assert(traits::has_switch_to_privileged_method_v<Policy>,
                  "ContextSwitch policy must provide switch_to_privileged()");
    static_assert(traits::has_dmb_method_v<Policy>, "ContextSwitch policy must provide dmb()");
    static_assert(traits::has_dsb_method_v<Policy>, "ContextSwitch policy must provide dsb()");
    static_assert(traits::has_isb_method_v<Policy>, "ContextSwitch policy must provide isb()");
    static_assert(traits::is_correct_get_psp_return_type_v<Policy>,
                  "ContextSwitch policy's get_psp() must return word");
    static_assert(traits::is_correct_get_msp_return_type_v<Policy>,
                  "ContextSwitch policy's get_msp() must return word");

    /// 栈指针基本类型（通常为 uint32_t）
    using word = typename Policy::word;

    /// 是否支持多处理器（编译期常量）
    static constexpr bool supports_multicore = traits::has_multicore_support_v<Policy>;

    /**
     * @brief 触发 PendSV 异常，用于延迟任务切换
     */
    inline static void trigger_pendsv() noexcept {
        Policy::trigger_pendsv();
    }

    /**
     * @brief 设置进程栈指针（PSP）
     * @param sp 栈指针值
     */
    inline static void set_psp(word sp) noexcept {
        Policy::set_psp(sp);
    }

    /**
     * @brief 获取进程栈指针（PSP）
     * @return 当前 PSP 值
     */
    [[nodiscard]] inline static word get_psp() noexcept {
        return Policy::get_psp();
    }

    /**
     * @brief 设置主栈指针（MSP）
     * @param sp 栈指针值
     */
    inline static void set_msp(word sp) noexcept {
        Policy::set_msp(sp);
    }

    /**
     * @brief 获取主栈指针（MSP）
     * @return 当前 MSP 值
     */
    [[nodiscard]] inline static word get_msp() noexcept {
        return Policy::get_msp();
    }

    /**
     * @brief 切换到用户模式（非特权级）
     */
    inline static void switch_to_unprivileged() noexcept {
        Policy::switch_to_unprivileged();
    }

    /**
     * @brief 切换到特权模式
     */
    inline static void switch_to_privileged() noexcept {
        Policy::switch_to_privileged();
    }

    /**
     * @brief 数据内存屏障（DMB）
     */
    inline static void dmb() noexcept {
        Policy::dmb();
    }

    /**
     * @brief 数据同步屏障（DSB）
     */
    inline static void dsb() noexcept {
        Policy::dsb();
    }

    /**
     * @brief 指令同步屏障（ISB）
     */
    inline static void isb() noexcept {
        Policy::isb();
    }

    // ----- 可选扩展方法（若策略提供则转发，否则不编译） -----

    /**
     * @brief 获取当前异常号（用于判断是否在中断中）
     * @return 异常号，0 表示线程模式 decltype(Policy::get_current_exception())
     * @note 仅当策略类提供 get_current_exception() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_get_current_exception_method_v<P>>>
    [[nodiscard]] inline static auto get_current_exception() noexcept -> decltype(Policy::get_current_exception()) {
        return Policy::get_current_exception();
    }

    /**
     * @brief 获取当前 CPU 核心 ID（多核支持）
     * @return 核心编号 decltype(Policy::core_id())
     * @note 仅当策略类提供 core_id() 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_core_id_method_v<P>>>
    [[nodiscard]] inline static auto core_id() noexcept -> decltype(Policy::core_id()) {
        return Policy::core_id();
    }

    /**
     * @brief 发送核间中断（IPI）到指定核心
     * @param target_core 目标核心编号
     * @note 仅当策略类提供 send_ipi(word) 时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_send_ipi_method_v<P>>>
    inline static void send_ipi(word target_core) noexcept {
        Policy::send_ipi(target_core);
    }
};

} // namespace strat_os::hal

#endif // STRATOS_HAL_CONTEXT_SWITCH_HPP