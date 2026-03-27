/**
 * @file StaticArray.hpp
 * @author YT_Minro (yurilt15312@outlook.com)
 * @brief 静态数组容器，适用于嵌入式高安全环境
 * @version 1.1.0
 * @date 2026-03-12
 *
 * @copyright Copyright (c) 2026
 *
 * @details
 * 提供编译期大小固定的数组容器，支持：
 * - 策略模式定制元素类型、大小类型、容量和对齐
 * - 可定制的错误处理（越界时调用）
 * - 基于索引的安全迭代器（解引用时调用 at()）
 * - 严格的拷贝/移动控制（防止意外复制）
 * - 零初始化保证
 * - 与 STL 兼容的接口（迭代器、size()、empty() 等）
 *
 * 该容器专为裸机嵌入式环境设计，无异常、无动态内存分配，
 * 所有操作均为 noexcept（除非元素赋值可能抛出）。
 *
 * @par 使用示例
 * @code
 * // 定义策略：8 个 uint16_t，使用 uint8_t 作为大小类型，2 字节对齐
 * using MyPolicy = mu_sstl::StaticAllocPolicy<uint16_t, uint8_t, 8, 2>;
 * // 定义数组类型，使用默认错误处理器（死循环）
 * using MyArray = mu_sstl::StaticArray<MyPolicy>;
 *
 * MyArray arr;
 * arr.fill(0xAA);                     // 全部填充
 * uint16_t x = arr.at(3);              // 安全访问
 * uint16_t y = arr[3];                  // 无检查访问（需确保索引有效）
 *
 * // 自定义错误处理器（必须永不返回）
 * struct MyErrorHandler {
 *     [[noreturn]] void operator()() const {
 *         // 记录错误、复位系统或进入死循环
 *         while (true) {}
 *     }
 * };
 * using SafeArray = mu_sstl::StaticArray<MyPolicy, MyErrorHandler>;
 * @endcode
 *
 * @warning 自定义 ErrorHandler 必须永不返回，否则 at() 行为未定义。
 * @warning 迭代器在容器销毁后失效，请勿在容器生命周期外使用。
 * @warning operator[] 不进行边界检查，仅用于性能敏感且已保证索引有效的场景。
 */

#pragma once

#ifndef MU_SSTL_STATIC_ARRAY_HPP
#define MU_SSTL_STATIC_ARRAY_HPP

#include "mu_sstl/errors/basic_error.hpp"
#include "mu_sstl/errors/kernel_handler_id.hpp"
#include <cstddef>
#include <cstdint>  // for uint8_t
#include <iterator> // for iterator tags
#include <type_traits>
#include <utility> // for std::forward

namespace mu_sstl
{
/**
 * @brief 平台最大对齐值
 *
 * 优先使用编译器扩展 __BIGGEST_ALIGNMENT__（若存在），否则回退到 alignof(std::max_align_t)。
 * 该值可用于默认对齐策略，确保能放置任何基础类型。
 */
#ifndef __BIGGEST_ALIGNMENT__
static constexpr std::size_t platform_max_align_t = alignof(std::max_align_t);
#else
static constexpr std::size_t platform_max_align_t = __BIGGEST_ALIGNMENT__;
#endif //__BIGGEST_ALIGNMENT__

namespace detail
{
// ----- 策略约束检测（用于 SFINAE 和 static_assert）-----

/** @brief 检测类型 T 是否包含内嵌类型 value_type */
template <typename T, typename = void>
struct has_value_type : std::false_type {};
template <typename T>
struct has_value_type<T, std::void_t<typename T::value_type>> : std::true_type {};

/** @brief 检测类型 T 是否包含内嵌类型 size_type */
template <typename T, typename = void>
struct has_size_type : std::false_type {};
template <typename T>
struct has_size_type<T, std::void_t<typename T::size_type>> : std::true_type {};

/** @brief 检测类型 T 是否包含静态成员 capacity */
template <typename T, typename = void>
struct has_capacity : std::false_type {};
template <typename T>
struct has_capacity<T, std::void_t<decltype(T::capacity)>> : std::true_type {};

/** @brief 检测类型 T 是否包含内嵌类型 difference_type */
template <typename T, typename = void>
struct has_difference_type : std::false_type {};
template <typename T>
struct has_difference_type<T, std::void_t<typename T::difference_type>> : std::true_type {};

/** @brief 检测类型 T 是否包含方法at() */
template <typename T, typename = void>
struct has_at_method : std::false_type {};
template <typename T>
struct has_at_method<T, std::void_t<decltype(std::declval<T>().at(std::declval<typename T::size_type>()))>>
    : std::true_type {};

// 便捷变量模板
template <typename T>
inline constexpr bool has_value_type_v = has_value_type<T>::value;
template <typename T>
inline constexpr bool has_size_type_v = has_size_type<T>::value;
template <typename T>
inline constexpr bool has_capacity_v = has_capacity<T>::value;
template <typename T>
inline constexpr bool has_difference_type_v = has_difference_type<T>::value;
template <typename T>
inline constexpr bool has_at_method_v = has_at_method<T>::value;

/** @brief 检查 size_type 是否为无符号整型 */
template <typename T, typename = void>
struct is_unsigned_size_type : std::false_type {};
template <typename T>
struct is_unsigned_size_type<T, std::void_t<typename T::size_type>> : std::is_unsigned<typename T::size_type> {};
template <typename T>
inline constexpr bool is_unsigned_size_type_v = is_unsigned_size_type<T>::value;

/** @brief 检查 capacity 是否为正数 */
template <typename T, typename = void>
struct capacity_positive : std::false_type {};
template <typename T>
struct capacity_positive<T, std::void_t<decltype(T::capacity)>> : std::integral_constant<bool, (T::capacity > 0)> {};
template <typename T>
inline constexpr bool capacity_positive_v = capacity_positive<T>::value;

/** @brief 检测策略是否提供 alignment 静态成员 */
template <typename T, typename = void>
struct has_alignment : std::false_type {};
template <typename T>
struct has_alignment<T, std::void_t<decltype(T::alignment)>> : std::true_type {};
template <typename T>
inline constexpr bool has_alignment_v = has_alignment<T>::value;

/** @brief 检查 alignment 是否为 2 的幂（有效对齐值） */
template <typename T, typename = void>
struct is_valid_alignment : std::false_type {};
template <typename T>
struct is_valid_alignment<T, std::void_t<decltype(T::alignment)>>
    : std::integral_constant<bool, (T::alignment & (T::alignment - 1)) == 0> {};
template <typename T>
inline constexpr bool is_valid_alignment_v = is_valid_alignment<T>::value;

// ----- 前向迭代器（索引版，不涉及指针算术）-----

/**
 * @brief 基于索引的前向迭代器，解引用时调用容器的 at() 保证边界安全
 *
 * @tparam Container 容器类型（必须提供 value_type、size_type、difference_type 和 at() 成员）
 * @tparam IsConst 是否为常迭代器
 */
template <typename Container,
          bool IsConst,
          typename = std::enable_if_t<detail::has_value_type_v<Container> && detail::has_size_type_v<Container> &&
                                      detail::has_difference_type_v<Container>>>
class IndexIterator_ {
    static_assert(detail::has_value_type_v<Container>, "Container must provide a value_type");
    static_assert(detail::has_size_type_v<Container>, "Container must provide a size_type");
    static_assert(detail::has_difference_type_v<Container>, "Container must provide a difference_type");
    static_assert(detail::has_at_method_v<Container>, "Container must provide an at() method");

  public:
    using container_type    = std::conditional_t<IsConst, const Container, Container>;
    using value_type        = typename Container::value_type;
    using reference         = std::conditional_t<IsConst, const value_type&, value_type&>;
    using pointer           = std::conditional_t<IsConst, const value_type*, value_type*>;
    using difference_type   = typename Container::difference_type;
    using iterator_category = std::forward_iterator_tag;
    using size_type         = typename Container::size_type;

    /** @brief 默认构造，迭代器处于无效状态（不可解引用） */
    IndexIterator_() = default;

    /**
     * @brief 构造迭代器指向容器中的指定索引
     * @param cont 指向容器的指针（必须有效且生命周期长于迭代器）
     * @param idx  索引值（应位于 [0, capacity] 范围内，end() 对应 capacity）
     */
    constexpr IndexIterator_(container_type* cont, size_type idx) noexcept
        : cont_(cont)
        , idx_(idx) {}

    /**
     * @brief 解引用迭代器，返回当前元素的引用
     * @return reference 元素引用
     * @note 内部调用 cont_->at(idx_)，确保边界安全
     */
    reference operator*() const {
        return cont_->at(idx_);
    }

    // 前向迭代器要求
    IndexIterator_& operator++() noexcept {
        ++idx_;
        return *this;
    }
    IndexIterator_ operator++(int) noexcept {
        auto tmp = *this;
        ++idx_;
        return tmp;
    }

    bool operator==(const IndexIterator_& other) const noexcept {
        return cont_ == other.cont_ && idx_ == other.idx_;
    }
    bool operator!=(const IndexIterator_& other) const noexcept {
        return !(*this == other);
    }

  private:
    container_type* cont_ = nullptr; ///< 指向容器的指针
    size_type idx_        = 0;       ///< 当前索引
};

} // namespace detail

// ----- 示例策略（用户可自定义）-----

/**
 * @brief 静态数组的默认策略，用户可据此自定义
 *
 * @tparam ValueType 元素类型
 * @tparam SizeType  大小类型（必须为无符号整型）
 * @tparam Capacity  容量（必须为正）
 * @tparam Alignment 对齐要求（必须是 2 的幂，默认 alignof(ValueType)）
 */
template <typename ValueType, typename SizeType, SizeType Capacity, std::size_t Alignment = alignof(ValueType)>
struct StaticAllocPolicy {
    using value_type                       = ValueType; ///< 元素类型
    using size_type                        = SizeType;  ///< 大小类型
    static constexpr SizeType capacity     = Capacity;  ///< 容量
    static constexpr std::size_t alignment = Alignment; ///< 对齐要求
};

/** @brief 默认策略实例：元素为 uint32_t，容量 80，对齐采用平台最大对齐 */
using DefaultAlloc = StaticAllocPolicy<std::uint32_t, std::size_t, 80, mu_sstl::platform_max_align_t>;

// ----- 受约束的 StaticArray -----

/**
 * @brief 静态数组容器，编译期固定大小，提供边界安全和可定制的错误处理
 *
 * @tparam AllocPolicy  分配策略，必须提供 value_type、size_type、capacity、alignment
 * @tparam ErrorHandler 错误处理器类型，默认 DefaultErrorHandler，必须可 const 调用且永不返回
 *
 * @details
 * 容器特点：
 * - 元素存储于内部数组，零初始化（默认构造全零）
 * - 提供 at() 边界检查，越界时调用 ErrorHandler
 * - 提供 operator[] 无检查快速访问
 * - 提供 data() 裸指针用于底层交互（需谨慎使用）
 * - 迭代器基于索引，解引用时调用 at() 保证安全
 * - 拷贝/移动操作被删除，防止意外复制
 * - 支持 fill() 和 swap()（仅同类型之间）
 *
 * @note 迭代器在容器销毁后失效，请勿在容器生命周期外使用
 * @warning 自定义 ErrorHandler 必须永不返回，否则 at() 行为未定义
 * @warning operator[] 不进行边界检查，仅用于性能敏感且已保证索引有效的场景
 */
template <typename AllocPolicy,
          typename ErrorHandler = FatalErrorHandler<kernel_handler_id::base_array_handler_id>,
          typename              = std::enable_if_t<
                           detail::has_value_type_v<AllocPolicy> && detail::has_size_type_v<AllocPolicy> &&
                           detail::has_capacity_v<AllocPolicy> && detail::is_unsigned_size_type_v<AllocPolicy> &&
                           detail::has_alignment_v<AllocPolicy> && detail::capacity_positive_v<AllocPolicy> &&
                           detail::is_valid_alignment_v<AllocPolicy> && mu_sstl::is_valid_error_handler_v<ErrorHandler>>>
class StaticArray {
    // ---------- 编译期断言：提供更清晰的错误信息 ----------
    static_assert(detail::has_value_type_v<AllocPolicy>, "AllocPolicy must provide a nested 'value_type' type");
    static_assert(detail::has_size_type_v<AllocPolicy>, "AllocPolicy must provide a nested 'size_type' type");
    static_assert(detail::has_capacity_v<AllocPolicy>, "AllocPolicy must provide a static 'capacity' member");
    static_assert(detail::is_unsigned_size_type_v<AllocPolicy>,
                  "AllocPolicy::size_type must be an unsigned integral type");
    static_assert(detail::capacity_positive_v<AllocPolicy>, "AllocPolicy::capacity must be positive");
    static_assert(detail::has_alignment_v<AllocPolicy>, "AllocPolicy must provide a static 'alignment' member");
    static_assert(detail::is_valid_alignment_v<AllocPolicy>, "AllocPolicy::alignment must be a power of two");
    static_assert(mu_sstl::is_valid_error_handler_v<ErrorHandler>,
                  "ErrorHandler must have is_fatal, id, and a const call operator() that never returns");

  public:
    // 类型别名（符合 STL 规范）
    using value_type      = typename AllocPolicy::value_type;
    using type            = value_type; ///< 兼容旧代码的别名
    using size_type       = typename AllocPolicy::size_type;
    using difference_type = std::ptrdiff_t;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;

    static_assert(std::is_object_v<value_type> && !std::is_abstract_v<value_type>,
                  "value_type must be a concrete (non-abstract) object type");
    // 检查 value_type 是否完整（防止前向声明类型）
    static_assert(static_cast<std::size_t>(sizeof(value_type)) > 0, "value_type must be a complete type");

    // 迭代器类型
    using iterator       = detail::IndexIterator_<StaticArray, false>;
    using const_iterator = detail::IndexIterator_<StaticArray, true>;

    // 容量常量
    static constexpr size_type capacity = AllocPolicy::capacity;
    static_assert(capacity > 0, "StaticArray capacity must be positive.");

    /** @brief 数组实际对齐值（取自策略） */
    static constexpr std::size_t alignment = AllocPolicy::alignment;

    // ----- 构造 / 析构 -----

    /** @brief 默认构造，零初始化所有元素 */
    StaticArray() = default;

    /**
     * @brief 接受 ErrorHandler 实例的构造函数
     * @tparam EH 错误处理器类型（必须可转换为 ErrorHandler）
     * @param eh  错误处理器实例（完美转发）
     */
    template <typename EH, typename = std::enable_if_t<std::is_constructible_v<ErrorHandler, EH&&>>>
    explicit StaticArray(EH&& eh_)
        : errorHandler_(std::forward<EH>(eh_)) {}

    // ----- 拷贝/移动控制 -----
    StaticArray(const StaticArray&)            = delete;
    StaticArray& operator=(const StaticArray&) = delete;
    StaticArray(StaticArray&&)                 = delete;
    StaticArray& operator=(StaticArray&&)      = delete;

    // ----- 元素访问 -----

    /**
     * @brief 带边界检查的访问
     * @param i 索引
     * @return 元素引用
     * @note 若 i >= capacity，调用 errorHandler()（必须永不返回）
     */
    [[nodiscard]] inline constexpr reference at(size_type i) {
        if (i >= capacity) errorHandler_(ErrorContext{ErrorCode::OutOfBounds, static_cast<std::uint32_t>(i)});
        return buffer_[i];
    }
    [[nodiscard]] inline constexpr const_reference at(size_type i) const {
        if (i >= capacity) errorHandler_(ErrorContext{ErrorCode::OutOfBounds, static_cast<std::uint32_t>(i)});
        return buffer_[i];
    }

    /** @brief 返回第一个元素的引用（未定义行为若容器为空） */
    [[nodiscard]] inline constexpr reference front() noexcept {
        return buffer_[0];
    }
    [[nodiscard]] inline constexpr const_reference front() const noexcept {
        return buffer_[0];
    }

    /** @brief 返回最后一个元素的引用（未定义行为若容器为空） */
    [[nodiscard]] inline constexpr reference back() noexcept {
        return buffer_[capacity - 1];
    }
    [[nodiscard]] inline constexpr const_reference back() const noexcept {
        return buffer_[capacity - 1];
    }

    /**
     * @brief 无边界检查的快速访问
     * @param i 索引（调用者必须保证 i < capacity）
     */
    [[nodiscard]] inline constexpr reference operator[](size_type i) noexcept {
        return buffer_[i];
    }
    [[nodiscard]] inline constexpr const_reference operator[](size_type i) const noexcept {
        return buffer_[i];
    }

    /**
     * @brief 用给定值填充所有元素
     * @param value 要填充的值
     */
    inline constexpr void fill(const value_type& value) noexcept {
        for (size_type i = 0; i < capacity; ++i) {
            buffer_[i] = value;
        }
    }

    /**
     * @brief 交换两个相同类型数组的内容
     * @param other 另一个 StaticArray 对象
     * @note 仅交换 buffer，不交换 errorHandler（设计如此，因为通常相同或无关）
     */
    void swap(StaticArray& other) noexcept {
        for (size_type i = 0; i < capacity; ++i) {
            using std::swap;
            swap(buffer_[i], other.buffer_[i]);
        }
    }

    /**
     * @brief 返回指向底层数组的裸指针（用于硬件交互）
     * @warning 使用此指针需格外小心，确保不越界且生命周期正确
     */
    [[nodiscard]] inline constexpr pointer data() noexcept {
        return buffer_;
    }
    [[nodiscard]] inline constexpr const_pointer data() const noexcept {
        return buffer_;
    }

    // ----- 容量信息 -----

    /** @brief 返回容器容量（始终等于 capacity） */
    [[nodiscard]] inline constexpr size_type size() const noexcept {
        return capacity;
    }

    /** @brief 返回最大可能大小（即 capacity） */
    [[nodiscard]] inline constexpr size_type max_size() const noexcept {
        return capacity;
    }

    /** @brief 返回容器占用的总字节数（sizeof(value_type) * capacity） */
    [[nodiscard]] inline constexpr std::size_t size_bytes() const noexcept {
        return sizeof(value_type) * capacity;
    }

    /** @brief 检查容器是否为空（容量为 0，但由于 static_assert，通常为 false） */
    [[nodiscard]] inline constexpr bool empty() const noexcept {
        return capacity == 0;
    }

    // ----- 迭代器 -----

    [[nodiscard]] iterator begin() noexcept {
        return iterator(this, 0);
    }
    [[nodiscard]] iterator end() noexcept {
        return iterator(this, capacity);
    }
    [[nodiscard]] const_iterator begin() const noexcept {
        return const_iterator(this, 0);
    }
    [[nodiscard]] const_iterator end() const noexcept {
        return const_iterator(this, capacity);
    }
    [[nodiscard]] const_iterator cbegin() const noexcept {
        return const_iterator(this, 0);
    }
    [[nodiscard]] const_iterator cend() const noexcept {
        return const_iterator(this, capacity);
    }

  private:
    alignas(alignment) value_type buffer_[capacity]{}; ///< 实际存储数组，零初始化
    ErrorHandler errorHandler_{};                      ///< 错误处理器实例
};

/**
 * @brief 非成员 swap 重载，支持 ADL 查找
 * @relates StaticArray
 */
template <typename AllocPolicy, typename ErrorHandler>
void swap(StaticArray<AllocPolicy, ErrorHandler>& a,
          StaticArray<AllocPolicy, ErrorHandler>& b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

} // namespace mu_sstl

#endif // MU_SSTL_STATIC_ARRAY_HPP
