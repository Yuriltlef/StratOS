/**
 * @file atomic.hpp
 * @author StratOS Team
 * @brief 原子操作策略接口与适配器
 * @version 1.1.0
 * @date 2026-03-31
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了硬件抽象层（HAL）的原子操作抽象接口，采用静态策略模式。
 * 它将原子操作（加载、存储、算术运算、比较交换、位操作等）封装为策略类，
 * 并通过类型萃取在编译期检测策略类是否满足接口约定，最终通过适配器模板
 * `Atomic` 提供统一的静态接口供 RTOS 内核使用。
 *
 * 策略类必须定义以下嵌套类型：
 * - value_type：原子操作的基本类型（无符号整数）
 * - bit_index_type：位索引的类型（无符号整数）
 *
 * 可选支持内存顺序（memory order）的策略可实现带 `std::memory_order` 参数的重载，
 * 适配器会通过 SFINAE 自动暴露相应接口。
 *
 * 该设计保证了零开销抽象，所有方法均为内联且 noexcept，适合嵌入式高安全环境。
 */
#pragma once

#ifndef STRATOS_HAL_ATOMIC_HPP
#define STRATOS_HAL_ATOMIC_HPP

#include <atomic>      // for std::memory_order
#include <type_traits> // for std::false_type, std::true_type, etc.
#include <utility>     // for std::declval

namespace strat_os::hal::traits
{

// -----------------------------------------------------------------------------
// 基础类型检测
// -----------------------------------------------------------------------------

/**
 * @brief 检测类型 T 是否包含嵌套类型 value_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_value_type : std::false_type {};
template <typename T>
struct has_value_type<T, std::void_t<typename T::value_type>> : std::true_type {};
template <typename T>
static constexpr bool has_value_type_v = has_value_type<T>::value;

/**
 * @brief 检测类型 T 是否包含嵌套类型 bit_index_type
 * @tparam T 待检测的类型
 */
template <typename T, typename = void>
struct has_bit_index_type : std::false_type {};
template <typename T>
struct has_bit_index_type<T, std::void_t<typename T::bit_index_type>> : std::true_type {};
template <typename T>
static constexpr bool has_bit_index_type_v = has_bit_index_type<T>::value;

/**
 * @brief 检测 value_type 是否为无符号整数类型
 * @note 仅当 T 包含 value_type 时使用
 */
template <typename T, typename = void>
struct is_valid_value_type : std::false_type {};
template <typename T>
struct is_valid_value_type<T, std::void_t<typename T::value_type>> : std::is_unsigned<typename T::value_type> {};
template <typename T>
static constexpr bool is_valid_value_type_v = is_valid_value_type<T>::value;

/**
 * @brief 检测 bit_index_type 是否为无符号整数类型
 * @note 仅当 T 包含 bit_index_type 时使用
 */
template <typename T, typename = void>
struct is_valid_bit_index_type : std::false_type {};
template <typename T>
struct is_valid_bit_index_type<T, std::void_t<typename T::bit_index_type>>
    : std::is_unsigned<typename T::bit_index_type> {};
template <typename T>
static constexpr bool is_valid_bit_index_type_v = is_valid_bit_index_type<T>::value;

// -----------------------------------------------------------------------------
// 方法存在性检测
// -----------------------------------------------------------------------------

/**
 * @brief 检测静态方法 load(volatile value_type*)
 */
template <typename T, typename = void>
struct has_load_method : std::false_type {};
template <typename T>
struct has_load_method<T, std::void_t<decltype(T::load(std::declval<volatile typename T::value_type*>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_load_method_v = has_load_method<T>::value;

/**
 * @brief 检测静态方法 store(volatile value_type*, value_type)
 */
template <typename T, typename = void>
struct has_store_method : std::false_type {};
template <typename T>
struct has_store_method<T,
                        std::void_t<decltype(T::store(std::declval<volatile typename T::value_type*>(),
                                                      std::declval<typename T::value_type>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_store_method_v = has_store_method<T>::value;

/**
 * @brief 检测静态方法 add(volatile value_type*, value_type)
 */
template <typename T, typename = void>
struct has_add_method : std::false_type {};
template <typename T>
struct has_add_method<T,
                      std::void_t<decltype(T::add(std::declval<volatile typename T::value_type*>(),
                                                  std::declval<typename T::value_type>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_add_method_v = has_add_method<T>::value;

/**
 * @brief 检测静态方法 sub(volatile value_type*, value_type)
 */
template <typename T, typename = void>
struct has_sub_method : std::false_type {};
template <typename T>
struct has_sub_method<T,
                      std::void_t<decltype(T::sub(std::declval<volatile typename T::value_type*>(),
                                                  std::declval<typename T::value_type>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_sub_method_v = has_sub_method<T>::value;

/**
 * @brief 检测静态方法 compare_exchange(volatile value_type*, value_type&, value_type)
 */
template <typename T, typename = void>
struct has_compare_exchange_method : std::false_type {};
template <typename T>
struct has_compare_exchange_method<
    T,
    std::void_t<decltype(T::compare_exchange(std::declval<volatile typename T::value_type*>(),
                                             std::declval<typename T::value_type&>(),
                                             std::declval<typename T::value_type>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_compare_exchange_method_v = has_compare_exchange_method<T>::value;

/**
 * @brief 检测静态方法 set_bit(volatile value_type*, bit_index_type)
 */
template <typename T, typename = void>
struct has_set_bit_method : std::false_type {};
template <typename T>
struct has_set_bit_method<T,
                          std::void_t<decltype(T::set_bit(std::declval<volatile typename T::value_type*>(),
                                                          std::declval<typename T::bit_index_type>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_set_bit_method_v = has_set_bit_method<T>::value;

/**
 * @brief 检测静态方法 clear_bit(volatile value_type*, bit_index_type)
 */
template <typename T, typename = void>
struct has_clear_bit_method : std::false_type {};
template <typename T>
struct has_clear_bit_method<T,
                            std::void_t<decltype(T::clear_bit(std::declval<volatile typename T::value_type*>(),
                                                              std::declval<typename T::bit_index_type>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_clear_bit_method_v = has_clear_bit_method<T>::value;

/**
 * @brief 检测静态方法 test_and_set_bit(volatile value_type*, bit_index_type)
 */
template <typename T, typename = void>
struct has_test_and_set_bit_method : std::false_type {};
template <typename T>
struct has_test_and_set_bit_method<
    T,
    std::void_t<decltype(T::test_and_set_bit(std::declval<volatile typename T::value_type*>(),
                                             std::declval<typename T::bit_index_type>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_test_and_set_bit_method_v = has_test_and_set_bit_method<T>::value;

/**
 * @brief 检测静态方法 flip_bit(volatile value_type*, bit_index_type)
 */
template <typename T, typename = void>
struct has_flip_bit_method : std::false_type {};
template <typename T>
struct has_flip_bit_method<T,
                           std::void_t<decltype(T::flip_bit(std::declval<volatile typename T::value_type*>(),
                                                            std::declval<typename T::bit_index_type>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_flip_bit_method_v = has_flip_bit_method<T>::value;

/**
 * @brief 检测静态方法 test_and_set(volatile value_type*)
 */
template <typename T, typename = void>
struct has_test_and_set_method : std::false_type {};
template <typename T>
struct has_test_and_set_method<T,
                               std::void_t<decltype(T::test_and_set(std::declval<volatile typename T::value_type*>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_test_and_set_method_v = has_test_and_set_method<T>::value;

// -----------------------------------------------------------------------------
// 返回值类型一致性检测
// -----------------------------------------------------------------------------

/**
 * @brief 检测 load() 的返回类型是否与 value_type 一致
 */
template <typename T>
struct is_correct_load_return_type
    : std::is_same<decltype(T::load(std::declval<volatile typename T::value_type*>())), typename T::value_type> {};
template <typename T>
static constexpr bool is_correct_load_return_type_v = is_correct_load_return_type<T>::value;

/**
 * @brief 检测 add() 的返回类型是否与 value_type 一致
 */
template <typename T>
struct is_correct_add_return_type : std::is_same<decltype(T::add(std::declval<volatile typename T::value_type*>(),
                                                                 std::declval<typename T::value_type>())),
                                                 typename T::value_type> {};
template <typename T>
static constexpr bool is_correct_add_return_type_v = is_correct_add_return_type<T>::value;

/**
 * @brief 检测 sub() 的返回类型是否与 value_type 一致
 */
template <typename T>
struct is_correct_sub_return_type : std::is_same<decltype(T::sub(std::declval<volatile typename T::value_type*>(),
                                                                 std::declval<typename T::value_type>())),
                                                 typename T::value_type> {};
template <typename T>
static constexpr bool is_correct_sub_return_type_v = is_correct_sub_return_type<T>::value;

// 对于带内存顺序的重载，同样检查返回类型一致性
/**
 * @brief 检测带内存顺序的 load() 返回类型是否与 value_type 一致
 */
template <typename T>
struct is_correct_load_memory_order_return_type
    : std::is_same<decltype(T::load(std::declval<volatile typename T::value_type*>(),
                                    std::declval<std::memory_order>())),
                   typename T::value_type> {};
template <typename T>
static constexpr bool is_correct_load_memory_order_return_type_v = is_correct_load_memory_order_return_type<T>::value;

/**
 * @brief 检测带内存顺序的 add() 返回类型是否与 value_type 一致
 */
template <typename T>
struct is_correct_add_memory_order_return_type
    : std::is_same<decltype(T::add(std::declval<volatile typename T::value_type*>(),
                                   std::declval<typename T::value_type>(),
                                   std::declval<std::memory_order>())),
                   typename T::value_type> {};
template <typename T>
static constexpr bool is_correct_add_memory_order_return_type_v = is_correct_add_memory_order_return_type<T>::value;

/**
 * @brief 检测带内存顺序的 sub() 返回类型是否与 value_type 一致
 */
template <typename T>
struct is_correct_sub_memory_order_return_type
    : std::is_same<decltype(T::sub(std::declval<volatile typename T::value_type*>(),
                                   std::declval<typename T::value_type>(),
                                   std::declval<std::memory_order>())),
                   typename T::value_type> {};
template <typename T>
static constexpr bool is_correct_sub_memory_order_return_type_v = is_correct_sub_memory_order_return_type<T>::value;

/**
 * @brief 检测不带内存顺序的 compare_exchange() 返回类型是否与 bool 一致
 */
template <typename T>
struct is_correct_compare_exchange_return_type
    : std::is_same<decltype(T::compare_exchange(std::declval<volatile typename T::value_type*>(),
                                                std::declval<typename T::value_type&>(),
                                                std::declval<typename T::value_type>())),
                   bool> {};
template <typename T>
static constexpr bool is_correct_compare_exchange_return_type_v = is_correct_compare_exchange_return_type<T>::value;

/**
 * @brief 检测带内存顺序的 compare_exchange() 返回类型是否与 value_type 一致
 */
template <typename T>
struct is_correct_compare_exchange_memory_order_return_type
    : std::is_same<decltype(T::compare_exchange(std::declval<volatile typename T::value_type*>(),
                                                std::declval<typename T::value_type&>(),
                                                std::declval<typename T::value_type>(),
                                                std::declval<std::memory_order>())),
                   bool> {};
template <typename T>
static constexpr bool is_correct_compare_exchange_memory_order_return_type_v =
    is_correct_compare_exchange_memory_order_return_type<T>::value;

/**
 * @brief 检测 test_and_set() 返回类型是否与 bool 一致
 */
template <typename T>
struct is_correct_test_and_set_return_type
    : std::is_same<decltype(T::test_and_set(std::declval<volatile typename T::value_type*>())), bool> {};
template <typename T>
static constexpr bool is_correct_test_and_set_return_type_v = is_correct_test_and_set_return_type<T>::value;

/**
 * @brief 检测 test_and_set_bit() 返回类型是否与 bool 一致
 */
template <typename T>
struct is_correct_test_and_set_bit_return_type
    : std::is_same<decltype(T::test_and_set_bit(std::declval<volatile typename T::value_type*>(),
                                                std::declval<typename T::bit_index_type>())),
                   bool> {};
template <typename T>
static constexpr bool is_correct_test_and_set_bit_return_type_v = is_correct_test_and_set_bit_return_type<T>::value;

// -----------------------------------------------------------------------------
// 组合检测：是否为有效的原子操作策略
// -----------------------------------------------------------------------------

/**
 * @brief 组合检测，判断类型 T 是否为有效的原子操作策略
 * @tparam T 待检测的类型
 *
 * 要求 T 必须提供：
 * - 嵌套类型 value_type（无符号整数）
 * - 嵌套类型 bit_index_type（无符号整数）
 * - 所有必需静态方法，且 `load`、`add`、`sub` 方法的返回类型必须与 value_type 一致
 *   （带内存顺序的重载同样需要满足返回值一致性）
 */
template <typename T>
struct is_valid_atomic_policy : std::conjunction<has_value_type<T>,
                                                 has_bit_index_type<T>,
                                                 is_valid_value_type<T>,
                                                 is_valid_bit_index_type<T>,
                                                 has_load_method<T>,
                                                 has_store_method<T>,
                                                 has_add_method<T>,
                                                 has_sub_method<T>,
                                                 has_compare_exchange_method<T>,
                                                 has_set_bit_method<T>,
                                                 has_clear_bit_method<T>,
                                                 has_test_and_set_bit_method<T>,
                                                 has_flip_bit_method<T>,
                                                 has_test_and_set_method<T>,
                                                 is_correct_load_return_type<T>,
                                                 is_correct_add_return_type<T>,
                                                 is_correct_sub_return_type<T>,
                                                 is_correct_compare_exchange_return_type<T>,
                                                 is_correct_test_and_set_return_type<T>,
                                                 is_correct_test_and_set_bit_return_type<T>> {};
template <typename T>
static constexpr bool is_valid_atomic_policy_v = is_valid_atomic_policy<T>::value;

// -----------------------------------------------------------------------------
// 内存顺序支持检测
// -----------------------------------------------------------------------------

/**
 * @brief 检测带内存顺序的 load(volatile value_type*, std::memory_order)
 */
template <typename T, typename = void>
struct has_load_memory_order_method : std::false_type {};
template <typename T>
struct has_load_memory_order_method<
    T,
    std::void_t<decltype(T::load(std::declval<volatile typename T::value_type*>(), std::declval<std::memory_order>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_load_memory_order_method_v = has_load_memory_order_method<T>::value;

/**
 * @brief 检测带内存顺序的 store(volatile value_type*, value_type, std::memory_order)
 */
template <typename T, typename = void>
struct has_store_memory_order_method : std::false_type {};
template <typename T>
struct has_store_memory_order_method<T,
                                     std::void_t<decltype(T::store(std::declval<volatile typename T::value_type*>(),
                                                                   std::declval<typename T::value_type>(),
                                                                   std::declval<std::memory_order>()))>>
    : std::true_type {};
template <typename T>
static constexpr bool has_store_memory_order_method_v = has_store_memory_order_method<T>::value;

/**
 * @brief 检测带内存顺序的 add(volatile value_type*, value_type, std::memory_order)
 */
template <typename T, typename = void>
struct has_add_memory_order_method : std::false_type {};
template <typename T>
struct has_add_memory_order_method<T,
                                   std::void_t<decltype(T::add(std::declval<volatile typename T::value_type*>(),
                                                               std::declval<typename T::value_type>(),
                                                               std::declval<std::memory_order>()))>> : std::true_type {
};
template <typename T>
static constexpr bool has_add_memory_order_method_v = has_add_memory_order_method<T>::value;

/**
 * @brief 检测带内存顺序的 sub(volatile value_type*, value_type, std::memory_order)
 */
template <typename T, typename = void>
struct has_sub_memory_order_method : std::false_type {};
template <typename T>
struct has_sub_memory_order_method<T,
                                   std::void_t<decltype(T::sub(std::declval<volatile typename T::value_type*>(),
                                                               std::declval<typename T::value_type>(),
                                                               std::declval<std::memory_order>()))>> : std::true_type {
};
template <typename T>
static constexpr bool has_sub_memory_order_method_v = has_sub_memory_order_method<T>::value;

/**
 * @brief 检测带内存顺序的 compare_exchange(volatile value_type*, value_type&, value_type, std::memory_order)
 */
template <typename T, typename = void>
struct has_compare_exchange_memory_order_method : std::false_type {};
template <typename T>
struct has_compare_exchange_memory_order_method<
    T,
    std::void_t<decltype(T::compare_exchange(std::declval<volatile typename T::value_type*>(),
                                             std::declval<typename T::value_type&>(),
                                             std::declval<typename T::value_type>(),
                                             std::declval<std::memory_order>()))>> : std::true_type {};
template <typename T>
static constexpr bool has_compare_exchange_memory_order_method_v = has_compare_exchange_memory_order_method<T>::value;

/**
 * @brief 聚合检测策略是否支持所有基本原子操作的内存顺序版本
 */
template <typename T>
struct is_memory_order_capable : std::conjunction<has_load_memory_order_method<T>,
                                                  has_store_memory_order_method<T>,
                                                  has_add_memory_order_method<T>,
                                                  has_sub_memory_order_method<T>,
                                                  has_compare_exchange_memory_order_method<T>> {};
template <typename T>
static constexpr bool is_memory_order_capable_v = is_memory_order_capable<T>::value;
} // namespace strat_os::hal::traits

namespace strat_os::hal
{

/**
 * @brief 原子操作适配器模板
 * @tparam AtomicPolicy 具体的策略类，必须满足原子操作策略接口
 *
 * 该类将策略类包装为统一的静态接口，并进行编译期验证。
 * 所有方法均为内联且 noexcept，转发到策略类的对应静态方法。
 *
 * 若策略支持内存顺序（通过 is_memory_order_capable 检测），则会提供带
 * std::memory_order 参数的重载版本；否则只提供无顺序的基本版本。
 *
 * 使用示例：
 * @code
 * using MyAtomic = Atomic<CortexM3Atomic>;
 * volatile uint32_t counter = 0;
 * MyAtomic::add(&counter, 1, std::memory_order_relaxed);
 * @endcode
 */
template <typename AtomicPolicy, typename = std::enable_if_t<traits::is_valid_atomic_policy_v<AtomicPolicy>>>
struct Atomic {
    /// 策略类别名
    using Policy = AtomicPolicy;

    // ----- 细粒度静态断言，提供清晰的错误信息 -----
    static_assert(traits::has_value_type_v<Policy>, "Atomic policy must provide a nested type 'value_type'");
    static_assert(traits::is_valid_value_type_v<Policy>, "Atomic policy::value_type must be an unsigned integer type");
    static_assert(traits::has_bit_index_type_v<Policy>, "Atomic policy must provide a nested type 'bit_index_type'");
    static_assert(traits::is_valid_bit_index_type_v<Policy>,
                  "Atomic policy::bit_index_type must be an unsigned integer type");
    static_assert(traits::has_load_method_v<Policy>, "Atomic policy must provide load(volatile value_type*)");
    static_assert(traits::is_correct_load_return_type_v<Policy>, "Atomic policy::load() must return value_type");
    static_assert(traits::has_store_method_v<Policy>,
                  "Atomic policy must provide store(volatile value_type*, value_type)");
    static_assert(traits::has_add_method_v<Policy>, "Atomic policy must provide add(volatile value_type*, value_type)");
    static_assert(traits::is_correct_add_return_type_v<Policy>, "Atomic policy::add() must return value_type");
    static_assert(traits::has_sub_method_v<Policy>, "Atomic policy must provide sub(volatile value_type*, value_type)");
    static_assert(traits::is_correct_sub_return_type_v<Policy>, "Atomic policy::sub() must return value_type");
    static_assert(traits::has_compare_exchange_method_v<Policy>,
                  "Atomic policy must provide compare_exchange(volatile value_type*, value_type&, value_type)");
    static_assert(traits::is_correct_compare_exchange_return_type_v<Policy>,
                  "Atomic policy::compare_exchange() must return value_type");
    static_assert(traits::has_set_bit_method_v<Policy>,
                  "Atomic policy must provide set_bit(volatile value_type*, bit_index_type)");
    static_assert(traits::has_clear_bit_method_v<Policy>,
                  "Atomic policy must provide clear_bit(volatile value_type*, bit_index_type)");
    static_assert(traits::has_test_and_set_bit_method_v<Policy>,
                  "Atomic policy must provide test_and_set_bit(volatile value_type*, bit_index_type)");
    static_assert(traits::has_flip_bit_method_v<Policy>,
                  "Atomic policy must provide flip_bit(volatile value_type*, bit_index_type)");
    static_assert(traits::has_test_and_set_method_v<Policy>,
                  "Atomic policy must provide test_and_set(volatile value_type*)");
    static_assert(traits::is_correct_test_and_set_return_type_v<Policy>,
                  "Atomic policy::test_and_set() must return bool");
    static_assert(traits::is_correct_test_and_set_bit_return_type_v<Policy>,
                  "Atomic policy::test_and_set_bit() must return bool");

    /// 原子操作的基本类型（无符号整数）
    using value_type = typename Policy::value_type;
    /// 位索引类型（无符号整数）
    using bit_index_type = typename Policy::bit_index_type;
    /// 内存顺序能力标志（编译期常量）
    static constexpr bool memory_order_capable = traits::is_memory_order_capable_v<Policy>;

    // ----- 基本读写（无内存顺序）-----

    /**
     * @brief 原子加载
     * @param ptr 指向 volatile 内存的指针
     * @return 当前值
     */
    [[nodiscard]] inline static value_type load(volatile value_type* ptr) noexcept {
        return Policy::load(ptr);
    }

    /**
     * @brief 原子存储
     * @param ptr   指向 volatile 内存的指针
     * @param value 要存储的值
     */
    inline static void store(volatile value_type* ptr, value_type value) noexcept {
        Policy::store(ptr, value);
    }

    // ----- 带内存顺序的读写（仅在策略支持时提供）-----

    /**
     * @brief 原子加载（带内存顺序）
     * @param ptr   指向 volatile 内存的指针
     * @param order 内存顺序（如 std::memory_order_acquire）
     * @return 当前值
     * @note 仅在策略支持内存顺序时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_load_memory_order_method_v<P>>>
    [[nodiscard]] inline static value_type load(volatile value_type* ptr, std::memory_order order) noexcept {
        static_assert(traits::is_correct_load_memory_order_return_type_v<P>,
                      "Atomic policy::load() with memory order must return value_type");
        return Policy::load(ptr, order);
    }

    /**
     * @brief 原子存储（带内存顺序）
     * @param ptr   指向 volatile 内存的指针
     * @param value 要存储的值
     * @param order 内存顺序（如 std::memory_order_release）
     * @note 仅在策略支持内存顺序时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_store_memory_order_method_v<P>>>
    inline static void store(volatile value_type* ptr, value_type value, std::memory_order order) noexcept {
        Policy::store(ptr, value, order);
    }

    // ----- 算术操作（无内存顺序）-----

    /**
     * @brief 原子加法
     * @param ptr   指向 volatile 内存的指针
     * @param value 要增加的值
     * @return 加后的新值
     */
    [[nodiscard]] inline static value_type add(volatile value_type* ptr, value_type value) noexcept {
        return Policy::add(ptr, value);
    }

    /**
     * @brief 原子减法
     * @param ptr   指向 volatile 内存的指针
     * @param value 要减少的值
     * @return 减后的新值
     */
    [[nodiscard]] inline static value_type sub(volatile value_type* ptr, value_type value) noexcept {
        return Policy::sub(ptr, value);
    }

    // ----- 带内存顺序的算术操作（仅在策略支持时提供）-----

    /**
     * @brief 原子加法（带内存顺序）
     * @param ptr   指向 volatile 内存的指针
     * @param value 要增加的值
     * @param order 内存顺序
     * @return 加后的新值
     * @note 仅在策略支持内存顺序时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_add_memory_order_method_v<P>>>
    [[nodiscard]] inline static value_type add(volatile value_type* ptr,
                                               value_type value,
                                               std::memory_order order) noexcept {
        static_assert(traits::is_correct_add_memory_order_return_type_v<P>,
                      "Atomic policy::add() with memory order must return value_type");
        return Policy::add(ptr, value, order);
    }

    /**
     * @brief 原子减法（带内存顺序）
     * @param ptr   指向 volatile 内存的指针
     * @param value 要减少的值
     * @param order 内存顺序
     * @return 减后的新值
     * @note 仅在策略支持内存顺序时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_sub_memory_order_method_v<P>>>
    [[nodiscard]] inline static value_type sub(volatile value_type* ptr,
                                               value_type value,
                                               std::memory_order order) noexcept {
        static_assert(traits::is_correct_sub_memory_order_return_type_v<P>,
                      "Atomic policy::sub() with memory order must return value_type");
        return Policy::sub(ptr, value, order);
    }

    // ----- 比较交换 -----

    /**
     * @brief 原子比较交换（无内存顺序）
     * @param ptr      指向 volatile 内存的指针
     * @param expected 期望值（若匹配则替换；若失败，该引用会被更新为当前值）
     * @param desired  目标值
     * @return true  如果成功写入
     * @return false 如果期望值不匹配（此时 *ptr 的值会写入 expected）
     */
    [[nodiscard]] inline static bool compare_exchange(volatile value_type* ptr,
                                                      value_type& expected,
                                                      value_type desired) noexcept {
        return Policy::compare_exchange(ptr, expected, desired);
    }

    /**
     * @brief 原子比较交换（带内存顺序）
     * @param ptr      指向 volatile 内存的指针
     * @param expected 期望值（若匹配则替换；若失败，该引用会被更新为当前值）
     * @param desired  目标值
     * @param order    内存顺序
     * @return true  如果成功写入
     * @return false 如果期望值不匹配
     * @note 仅在策略支持内存顺序时可用
     */
    template <typename P = Policy, typename = std::enable_if_t<traits::has_compare_exchange_memory_order_method_v<P>>>
    [[nodiscard]] inline static bool compare_exchange(volatile value_type* ptr,
                                                      value_type& expected,
                                                      value_type desired,
                                                      std::memory_order order) noexcept {
        static_assert(traits::is_correct_compare_exchange_memory_order_return_type_v<P>,
                      "Atomic policy::compare_exchange() with memory order must return bool");
        return Policy::compare_exchange(ptr, expected, desired, order);
    }

    // ----- 位操作（无内存顺序，因为通常只需原子性）-----

    /**
     * @brief 原子地设置指定位为 1
     * @param ptr 指向 volatile 内存的指针
     * @param bit 位索引
     */
    inline static void set_bit(volatile value_type* ptr, bit_index_type bit) noexcept {
        Policy::set_bit(ptr, bit);
    }

    /**
     * @brief 原子地清除指定位为 0
     * @param ptr 指向 volatile 内存的指针
     * @param bit 位索引
     */
    inline static void clear_bit(volatile value_type* ptr, bit_index_type bit) noexcept {
        Policy::clear_bit(ptr, bit);
    }

    /**
     * @brief 原子地测试并设置指定位（返回旧值，并置为 1）
     * @param ptr 指向 volatile 内存的指针
     * @param bit 位索引
     * @return true  原值为 1
     * @return false 原值为 0
     */
    [[nodiscard]] inline static bool test_and_set_bit(volatile value_type* ptr, bit_index_type bit) noexcept {
        return Policy::test_and_set_bit(ptr, bit);
    }

    /**
     * @brief 原子地翻转指定位
     * @param ptr 指向 volatile 内存的指针
     * @param bit 位索引
     */
    inline static void flip_bit(volatile value_type* ptr, bit_index_type bit) noexcept {
        Policy::flip_bit(ptr, bit);
    }

    // ----- 测试并设置（用于自旋锁）-----

    /**
     * @brief 原子测试并设置整个字（返回旧值，并置为 1）
     * @param ptr 指向 volatile 内存的指针
     * @return true  原值为 1
     * @return false 原值为 0
     * @note 常用于自旋锁的实现。
     */
    [[nodiscard]] inline static bool test_and_set(volatile value_type* ptr) noexcept {
        return Policy::test_and_set(ptr);
    }
};

} // namespace strat_os::hal

#endif // STRATOS_HAL_ATOMIC_HPP