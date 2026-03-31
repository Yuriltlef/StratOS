/**
 * @file system_control.hpp
 * @author StratOS Team
 * @brief 系统控制策略接口与适配器
 * @version 1.0.0
 * @date 2026-03-28
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了硬件抽象层（HAL）的系统控制抽象接口，采用静态策略模式。
 * 它将与系统级控制相关的硬件操作（如复位、向量表重定位、优先级分组、
 * 睡眠控制、异常优先级、故障处理等）封装为策略类，并通过类型萃取在
 * 编译期验证策略的完整性，最终通过适配器模板提供统一的静态接口供
 * RTOS 内核使用。
 *
 * 该设计保证了零开销抽象，所有方法均为内联且 noexcept，适合嵌入式高安全环境。
 *
 * 主要组件：
 * - traits 命名空间：提供类型萃取，用于检测策略类是否满足接口约定。
 * - SystemControl 适配器模板：接收一个策略类，进行编译期验证，并转发所有操作。
 *
 * 策略类需定义以下嵌套类型：
 *          - exception_type：异常编号的类型（如 CMSIS 的 IRQn_Type）
 *          - priority_type：优先级值的类型（如 uint8_t）
 *          - address_type：向量表地址的类型（通常为 uint32_t 或 uint64_t）
 *          - priority_group_type：优先级分组值的类型（通常为 uint32_t）
 *          - fault_mask_type：故障使能掩码的类型（通常为 uint32_t）
 *
 * 可选功能（如故障处理）通过 SFINAE 自动提供，策略可返回任意类型（利用 auto 推导）。
 *
 * @note 策略类必须提供所有必需静态方法（system_reset、set_vector_table、
 *       set_priority_grouping、get_priority_grouping、sleep、deep_sleep、
 *       set_sleep_on_exit、set_exception_priority、get_exception_priority）。
 *       可选方法（enable_faults、disable_faults、get_fault_info）可根据硬件能力选择性实现。
 *
 * @warning 所有方法均假设在特权模式下调用，且调用前已确保系统状态合法。
 */
#pragma once

#ifndef STRATOS_HAL_SYSTEM_CONTROL_HPP
#define STRATOS_HAL_SYSTEM_CONTROL_HPP

#include <type_traits> // for std::false_type, std::true_type, etc.
#include <utility>     // for std::declval

namespace strat_os::hal::traits
{
/**
 * @brief 检测类型 T 是否包含嵌套类型 exception_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_exception_type : std::false_type {};
template <typename T>
struct has_exception_type<T, std::void_t<typename T::exception_type>> : std::true_type {};
template <typename T>
inline constexpr bool has_exception_type_v = has_exception_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 priority_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_priority_type : std::false_type {};
template <typename T>
struct has_priority_type<T, std::void_t<typename T::priority_type>> : std::true_type {};
template <typename T>
inline constexpr bool has_priority_type_v = has_priority_type<T>::value;

/**
 * @brief 检测类型 T 的 priority_type 是否为无符号整数类型
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_valid_priority_type : std::is_unsigned<typename T::priority_type> {};
template <typename T>
inline constexpr bool is_valid_priority_type_v = is_valid_priority_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 address_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_address_type : std::false_type {};
template <typename T>
struct has_address_type<T, std::void_t<typename T::address_type>> : std::true_type {};
template <typename T>
inline constexpr bool has_address_type_v = has_address_type<T>::value;

/**
 * @brief 检测类型 T 的 address_type 是否为无符号整数类型
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_valid_address_type : std::is_unsigned<typename T::address_type> {};
template <typename T>
inline constexpr bool is_valid_address_type_v = is_valid_address_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 priority_group_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_priority_group_type : std::false_type {};
template <typename T>
struct has_priority_group_type<T, std::void_t<typename T::priority_group_type>> : std::true_type {};
template <typename T>
inline constexpr bool has_priority_group_type_v = has_priority_group_type<T>::value;

/**
 * @brief 检测类型 T 的 priority_group_type 是否为无符号整数类型
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_valid_priority_group_type : std::is_unsigned<typename T::priority_group_type> {};
template <typename T>
inline constexpr bool is_valid_priority_group_type_v = is_valid_priority_group_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 fault_mask_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_fault_mask_type : std::false_type {};
template <typename T>
struct has_fault_mask_type<T, std::void_t<typename T::fault_mask_type>> : std::true_type {};
template <typename T>
inline constexpr bool has_fault_mask_type_v = has_fault_mask_type<T>::value;

/**
 * @brief 检测类型 T 的 fault_mask_type 是否为无符号整数类型
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_valid_fault_mask_type : std::is_unsigned<typename T::fault_mask_type> {};
template <typename T>
inline constexpr bool is_valid_fault_mask_type_v = is_valid_fault_mask_type<T>::value;

// ---------- 必需方法检测 ----------
/**
 * @brief 检测类型 T 是否包含方法 system_reset()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_system_reset : std::false_type {};
template <typename T>
struct has_system_reset<T, std::void_t<decltype(T::system_reset())>> : std::true_type {};
template <typename T>
inline constexpr bool has_system_reset_v = has_system_reset<T>::value;

/**
 * @brief 检测类型 T 是否包含方法 set_vector_table(T::address_type)
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_set_vector_table : std::false_type {};
template <typename T>
struct has_set_vector_table<T, std::void_t<decltype(T::set_vector_table(std::declval<typename T::address_type>()))>>
    : std::true_type {};
template <typename T>
inline constexpr bool has_set_vector_table_v = has_set_vector_table<T>::value;

/**
 * @brief 检测类型 T 是否包含方法 set_priority_grouping(T::priority_group_type)
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_set_priority_grouping : std::false_type {};
template <typename T>
struct has_set_priority_grouping<
    T,
    std::void_t<decltype(T::set_priority_grouping(std::declval<typename T::priority_group_type>()))>> : std::true_type {
};
template <typename T>
inline constexpr bool has_set_priority_grouping_v = has_set_priority_grouping<T>::value;

/**
 * @brief 检测类型 T 是否包含方法 get_priority_grouping() -> T::priority_group_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_get_priority_grouping : std::false_type {};
template <typename T>
struct has_get_priority_grouping<T, std::void_t<decltype(T::get_priority_grouping())>> : std::true_type {};
template <typename T>
inline constexpr bool has_get_priority_grouping_v = has_get_priority_grouping<T>::value;

/**
 * @brief 检测类型 T 的 get_priority_grouping() 返回类型是否为 priority_group_type
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_correct_get_priority_grouping_return_type
    : std::is_same<decltype(T::get_priority_grouping()), typename T::priority_group_type> {};
template <typename T>
inline constexpr bool is_correct_get_priority_grouping_return_type_v =
    is_correct_get_priority_grouping_return_type<T>::value;

/**
 * @brief 检测类型 T 是否包含方法 sleep()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_sleep : std::false_type {};
template <typename T>
struct has_sleep<T, std::void_t<decltype(T::sleep())>> : std::true_type {};
template <typename T>
inline constexpr bool has_sleep_v = has_sleep<T>::value;

/**
 * @brief 检测类型 T 是否包含方法 deep_sleep()
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_deep_sleep : std::false_type {};
template <typename T>
struct has_deep_sleep<T, std::void_t<decltype(T::deep_sleep())>> : std::true_type {};
template <typename T>
inline constexpr bool has_deep_sleep_v = has_deep_sleep<T>::value;

/**
 * @brief 检测类型 T 是否包含方法 set_sleep_on_exit(bool)
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_set_sleep_on_exit : std::false_type {};
template <typename T>
struct has_set_sleep_on_exit<T, std::void_t<decltype(T::set_sleep_on_exit(std::declval<bool>()))>> : std::true_type {};
template <typename T>
inline constexpr bool has_set_sleep_on_exit_v = has_set_sleep_on_exit<T>::value;

// 异常优先级相关方法（使用策略定义的 exception_type 和 priority_type）
/**
 * @brief 检测类型 T 是否包含方法 set_exception_priority(T::exception_type, T::priority_type)
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_set_exception_priority : std::false_type {};
template <typename T>
struct has_set_exception_priority<
    T,
    std::void_t<decltype(T::set_exception_priority(std::declval<typename T::exception_type>(),
                                                   std::declval<typename T::priority_type>()))>> : std::true_type {};
template <typename T>
inline constexpr bool has_set_exception_priority_v = has_set_exception_priority<T>::value;

/**
 * @brief 检测类型 T 是否包含方法 get_exception_priority(T::exception_type) -> T::priority_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_get_exception_priority : std::false_type {};
template <typename T>
struct has_get_exception_priority<
    T,
    std::void_t<decltype(T::get_exception_priority(std::declval<typename T::exception_type>()))>> : std::true_type {};
template <typename T>
inline constexpr bool has_get_exception_priority_v = has_get_exception_priority<T>::value;

/**
 * @brief 检测类型 T 的 get_exception_priority() 返回类型是否为 priority_type
 * @tparam T 待检测的类型
 */
template <typename T>
struct is_correct_get_exception_priority_return_type
    : std::is_same<decltype(T::get_exception_priority(std::declval<typename T::exception_type>())),
                   typename T::priority_type> {};
template <typename T>
inline constexpr bool is_correct_get_exception_priority_return_type_v =
    is_correct_get_exception_priority_return_type<T>::value;

// ---------- 可选方法检测 ----------
/**
 * @brief 检测类型 T 是否包含方法 enable_faults(T::fault_mask_type)
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_enable_faults : std::false_type {};
template <typename T>
struct has_enable_faults<T, std::void_t<decltype(T::enable_faults(std::declval<typename T::fault_mask_type>()))>>
    : std::true_type {};
template <typename T>
inline constexpr bool has_enable_faults_v = has_enable_faults<T>::value;

/**
 * @brief 检测类型 T 是否包含方法 disable_faults(T::fault_mask_type)
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_disable_faults : std::false_type {};
template <typename T>
struct has_disable_faults<T, std::void_t<decltype(T::disable_faults(std::declval<typename T::fault_mask_type>()))>>
    : std::true_type {};
template <typename T>
inline constexpr bool has_disable_faults_v = has_disable_faults<T>::value;

// 检测 get_fault_info() – 返回任意类型（通过 decltype 推断）
template <typename T, typename = void>
struct has_get_fault_info : std::false_type {};
template <typename T>
struct has_get_fault_info<T, std::void_t<decltype(T::get_fault_info())>> : std::true_type {};
template <typename T>
inline constexpr bool has_get_fault_info_v = has_get_fault_info<T>::value;

// ---------- 组合检测：是否为有效的系统控制策略 ----------
template <typename T>
struct is_valid_system_control_policy : std::conjunction<has_exception_type<T>,
                                                         has_priority_type<T>,
                                                         has_address_type<T>,
                                                         has_priority_group_type<T>,
                                                         has_fault_mask_type<T>,
                                                         is_valid_priority_type<T>,
                                                         is_valid_address_type<T>,
                                                         is_valid_priority_group_type<T>,
                                                         is_valid_fault_mask_type<T>,
                                                         has_system_reset<T>,
                                                         has_set_vector_table<T>,
                                                         has_set_priority_grouping<T>,
                                                         has_get_priority_grouping<T>,
                                                         has_sleep<T>,
                                                         has_deep_sleep<T>,
                                                         has_set_sleep_on_exit<T>,
                                                         has_set_exception_priority<T>,
                                                         has_get_exception_priority<T>,
                                                         is_correct_get_priority_grouping_return_type<T>,
                                                         is_correct_get_exception_priority_return_type<T>> {};

template <typename T>
inline constexpr bool is_valid_system_control_policy_v = is_valid_system_control_policy<T>::value;

/**
 * @brief 组合检测：判断类型 T 是否为增强型系统控制策略
 * @tparam T 待检测的类型
 *
 * 增强型系统控制策略需同时提供 enable_faults、disable_faults 和 get_fault_info 方法。
 */
template <typename T>
struct is_enhanced_fault_controller
    : std::conjunction<has_enable_faults<T>, has_disable_faults<T>, has_get_fault_info<T>> {};

template <typename T>
inline constexpr bool is_enhanced_fault_controller_v = is_enhanced_fault_controller<T>::value;

} // namespace strat_os::hal::traits

namespace strat_os::hal
{

/**
 * @brief 系统控制适配器模板
 * @tparam SystemPolicy 具体的策略类，必须满足系统控制策略接口
 *
 * 该类将策略类包装为统一的静态接口，并进行编译期验证。
 * 所有方法均为内联且 noexcept，转发到策略类的对应静态方法。
 *
 * 策略类需定义 exception_type 和 priority_type 嵌套类型。
 *
 * 使用示例：
 * @code
 * using MySystem = SystemControl<CortexM3SystemControl>;
 * MySystem::system_reset();
 * MySystem::set_vector_table(0x08000000);
 * MySystem::set_exception_priority(PendSV_IRQn, 15);
 * @endcode
 */
template <typename SystemPolicy, typename = std::enable_if_t<traits::is_valid_system_control_policy_v<SystemPolicy>>>
struct SystemControl {
    using Policy = SystemPolicy;

    /// 细粒度静态断言，提供清晰的错误信息
    static_assert(traits::has_exception_type_v<Policy>, "Policy must define 'exception_type'");
    static_assert(traits::has_priority_type_v<Policy>, "Policy must define 'priority_type'");
    static_assert(traits::has_address_type_v<Policy>, "Policy must define 'address_type'");
    static_assert(traits::has_priority_group_type_v<Policy>, "Policy must define 'priority_group_type'");
    static_assert(traits::has_fault_mask_type_v<Policy>, "Policy must define 'fault_mask_type'");
    static_assert(traits::is_valid_priority_type_v<Policy>, "Policy's 'priority_type' must be an unsigned integer");
    static_assert(traits::is_valid_address_type_v<Policy>, "Policy's 'address_type' must be an unsigned integer");
    static_assert(traits::is_valid_priority_group_type_v<Policy>,
                  "Policy's 'priority_group_type' must be an unsigned integer");
    static_assert(traits::is_valid_fault_mask_type_v<Policy>, "Policy's 'fault_mask_type' must be an unsigned integer");
    static_assert(traits::has_system_reset_v<Policy>, "Policy must provide system_reset()");
    static_assert(traits::has_set_vector_table_v<Policy>, "Policy must provide set_vector_table()");
    static_assert(traits::has_set_priority_grouping_v<Policy>, "Policy must provide set_priority_grouping()");
    static_assert(traits::has_get_priority_grouping_v<Policy>, "Policy must provide get_priority_grouping()");
    static_assert(traits::has_sleep_v<Policy>, "Policy must provide sleep()");
    static_assert(traits::has_deep_sleep_v<Policy>, "Policy must provide deep_sleep()");
    static_assert(traits::has_set_sleep_on_exit_v<Policy>, "Policy must provide set_sleep_on_exit()");
    static_assert(traits::has_set_exception_priority_v<Policy>,
                  "Policy must provide set_exception_priority(exception_type, priority_type)");
    static_assert(traits::has_get_exception_priority_v<Policy>,
                  "Policy must provide get_exception_priority(exception_type)");
    static_assert(traits::is_correct_get_priority_grouping_return_type_v<Policy>,
                  "Policy's get_priority_grouping() must return priority_group_type");
    static_assert(traits::is_correct_get_exception_priority_return_type_v<Policy>,
                  "Policy's get_exception_priority() must return priority_type");

    /// 类型别名，供外部使用
    using exception_type      = typename Policy::exception_type;
    using priority_type       = typename Policy::priority_type;
    using address_type        = typename Policy::address_type;
    using priority_group_type = typename Policy::priority_group_type;
    using fault_mask_type     = typename Policy::fault_mask_type;

    /// 是否支持增强故障处理
    static constexpr bool enhanced_fault_controller = traits::is_enhanced_fault_controller_v<Policy>;

    // ----- 必需操作 -----
    /**
     * @brief 触发系统复位
     */
    inline static void system_reset() noexcept {
        Policy::system_reset();
    }

    /**
     * @brief 设置向量表基址
     * @param address 向量表起始地址（必须对齐到 256 字节或更大）
     */
    inline static void set_vector_table(address_type address) noexcept {
        Policy::set_vector_table(address);
    }

    /**
     * @brief 设置中断优先级分组
     * @param group 优先级分组值（0～7，对应不同的抢占优先级位数）
     * @note 具体分组值与抢占优先级/子优先级的对应关系见芯片手册。
     */
    inline static void set_priority_grouping(priority_group_type group) noexcept {
        Policy::set_priority_grouping(group);
    }

    /**
     * @brief 获取当前中断优先级分组
     * @return 优先级分组值
     */
    [[nodiscard]] inline static priority_group_type get_priority_grouping() noexcept {
        return Policy::get_priority_grouping();
    }

    /**
     * @brief 进入普通睡眠模式（WFI）
     */
    inline static void sleep() noexcept {
        Policy::sleep();
    }

    /**
     * @brief 进入深度睡眠模式（设置 SLEEPDEEP 后 WFI）
     * @note 实际功耗模式取决于电源管理单元的配置。
     */
    inline static void deep_sleep() noexcept {
        Policy::deep_sleep();
    }

    /**
     * @brief 设置中断返回后自动进入睡眠模式
     * @param enable true: 使能 SLEEPONEXIT, false: 禁用
     */
    inline static void set_sleep_on_exit(bool enable) noexcept {
        Policy::set_sleep_on_exit(enable);
    }

    /**
     * @brief 设置系统异常优先级
     * @param exception 异常编号（类型由策略定义）
     * @param priority  优先级值（数值越小优先级越高）
     */
    inline static void set_exception_priority(exception_type exception, priority_type priority) noexcept {
        Policy::set_exception_priority(exception, priority);
    }

    /**
     * @brief 获取系统异常优先级
     * @param exception 异常编号
     * @return 优先级值
     */
    [[nodiscard]] inline static priority_type get_exception_priority(exception_type exception) noexcept {
        return Policy::get_exception_priority(exception);
    }

    // ----- 可选：故障控制 -----
    /**
     * @brief 使能指定的可配置故障异常（MemManage、BusFault、UsageFault）
     * @param faults 故障掩码（例如 SCB_SHCSR_MEMFAULTENA_Msk 等）
     * @note 仅当策略支持故障处理时可用。
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_enable_faults_v<P>>>
    inline static void enable_faults(fault_mask_type faults) noexcept {
        Policy::enable_faults(faults);
    }

    /**
     * @brief 禁用指定的可配置故障异常
     * @param faults 故障掩码
     * @note 仅当策略支持故障处理时可用。
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_disable_faults_v<P>>>
    inline static void disable_faults(fault_mask_type faults) noexcept {
        Policy::disable_faults(faults);
    }

    // ----- 可选：获取故障信息（返回类型由策略决定，完美转发）-----
    /**
     * @brief 获取故障信息（具体内容由策略定义）
     * @return 故障信息对象（类型由策略决定）
     * @note 仅当策略提供 get_fault_info() 时可用。
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_get_fault_info_v<P>>>
    [[nodiscard]] inline static auto get_fault_info() noexcept -> decltype(Policy::get_fault_info()) {
        return Policy::get_fault_info();
    }
};

} // namespace strat_os::hal

#endif // STRATOS_HAL_SYSTEM_CONTROL_HPP