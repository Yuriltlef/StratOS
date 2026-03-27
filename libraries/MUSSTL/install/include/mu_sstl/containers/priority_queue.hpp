/**
 * @file priority_queue.hpp
 * @author YT_Minro (yurilt15312@outlook.com)
 * @brief 静态优先级队列容器，适用于嵌入式高安全环境
 * @version 1.0.0
 * @date 2026-03-18
 *
 * @copyright Copyright (c) 2026
 *
 * @details
 * 提供编译期大小固定的优先级队列，基于 StaticArray 实现堆结构。
 * 支持最大堆（MaxHeap）和最小堆（MinHeap）两种模式，通过模板参数 QueueType 选择。
 * 内部使用堆算法（上浮/下沉），所有操作均为 O(log n) 或 O(1)。
 * 主要特性：
 * - 固定容量，无动态内存分配，行为可预测
 * - 模板参数 AllocPolicy 指定元素类型、索引类型、容量和堆类型（通过内嵌 queue_type）
 * - 可定制的错误处理器（满、空时调用），默认处理器使用 FatalErrorHandler 并关联内核 ID
 * - 严格的拷贝/移动控制（禁止拷贝和移动，确保唯一所有权）
 * - 零初始化保证（默认构造所有元素清零）
 * - 提供容量查询（size、empty、full、available）、顶端访问、插入、删除、清空等操作
 *
 * 该容器专为裸机嵌入式 RTOS 设计，用于定时器管理、优先级调度等场景。
 * 所有不调用错误处理器的操作均为 noexcept。
 *
 * @par 使用示例
 * @code
 * #include "mu_sstl/containers/priority_queue.hpp"
 *
 * // 定义分配策略：int 类型，容量 16，最大堆
 * using MyPolicy = mu_sstl::PriorityQueueAllocPolicy<int, std::size_t, 16, mu_sstl::QueueType::MaxHeap>;
 * // 定义队列类型，使用默认错误处理器
 * using MyPQ = mu_sstl::PriorityQueue<MyPolicy>;
 *
 * MyPQ pq;
 *
 * // 插入元素
 * pq.push(10);
 * pq.push(5);
 * pq.push(20);
 *
 * // 访问顶端
 * int top = pq.top();  // 20（最大堆）
 *
 * // 弹出顶端
 * pq.pop();            // 移除 20
 *
 * // 此时 top 变为 10
 *
 * // 自定义错误处理器（必须永不返回）
 * struct MyErrorHandler {
 *     static constexpr bool is_fatal = true;
 *     static constexpr std::uint32_t id = 0x300;
 *     [[noreturn]] void operator()(const mu_sstl::ErrorContext& ctx) const {
 *         // 记录错误、复位系统或进入死循环
 *         while (true) {}
 *     }
 * };
 * mu_sstl::PriorityQueue<MyPolicy, MyErrorHandler> safe_pq;
 * @endcode
 *
 * @warning 自定义 ErrorHandler 必须满足 is_valid_error_handler_v 的要求：
 *          - 提供静态成员 is_fatal (bool) 和 id (可转为 uint16_t)
 *          - 提供 const 限定的 operator()(const ErrorContext&) 且永不返回
 *          否则在满、空等错误时将产生未定义行为。
 * @note 该类禁止拷贝和移动，因为底层存储不可复制（StaticArray 已禁用拷贝/移动）。
 * @note 比较操作依赖于元素类型的 operator< 或 operator>，具体由堆类型决定：
 *       - 最大堆使用 operator<
 *       - 最小堆使用 operator>
 *       用户必须确保元素类型重载了相应的操作符。
 */
#pragma once

#ifndef MU_SSTL_PRIORITY_QUEUE_HPP
#define MU_SSTL_PRIORITY_QUEUE_HPP

#include "mu_sstl/containers/static_array.hpp"
#include "mu_sstl/errors/basic_error.hpp"
#include "mu_sstl/errors/kernel_handler_id.hpp"
#include <cstddef> // for std::size_t
#include <cstdint> // for uint32_t
#include <type_traits>

namespace mu_sstl
{

/**
 * @brief 堆类型枚举
 *
 * 用于指定优先级队列是最大堆还是最小堆。
 */
enum class QueueType : std::uint8_t {
    MinHeap, ///< 最小堆：顶端元素最小
    MaxHeap  ///< 最大堆：顶端元素最大
};

namespace detail
{

/**
 * @brief 检测类型 T 是否包含静态成员 queue_type
 */
template <typename T, typename = void>
struct has_queue_type : std::false_type {};

template <typename T>
struct has_queue_type<T, std::void_t<decltype(T::queue_type)>> : std::true_type {};

/**
 * @brief 检测类型 T 的 queue_type 是否为 QueueType 类型
 */
template <typename T, typename = void>
struct is_valid_queue_type : std::false_type {};

template <typename T>
struct is_valid_queue_type<T, std::void_t<decltype(T::queue_type)>>
    : std::integral_constant<bool, std::is_same_v<std::remove_cv_t<decltype(T::queue_type)>, QueueType>> {};

template <typename T>
inline constexpr bool has_queue_type_v = has_queue_type<T>::value;
template <typename T>
inline constexpr bool is_valid_queue_type_v = is_valid_queue_type<T>::value;

/**
 * @brief 堆比较器模板，根据堆类型特化
 * @tparam Type 堆类型（QueueType::MaxHeap 或 QueueType::MinHeap）
 */
template <QueueType Type>
struct HeapCompare_ {};

/**
 * @brief 最大堆比较器特化
 * 使用 operator< 判断父子关系：当父节点小于子节点时应交换
 */
template <>
struct HeapCompare_<QueueType::MaxHeap> {
    template <typename T>
    constexpr bool operator()(const T& a, const T& b) const {
        return a < b; // 最大堆：父节点应大于子节点，所以当 a < b 时需要交换
    }
};

/**
 * @brief 最小堆比较器特化
 * 使用 operator> 判断父子关系：当父节点大于子节点时应交换
 */
template <>
struct HeapCompare_<QueueType::MinHeap> {
    template <typename T>
    constexpr bool operator()(const T& a, const T& b) const {
        return a > b; // 最小堆：父节点应小于子节点，所以当 a > b 时需要交换
    }
};
} // namespace detail

/**
 * @brief 优先级队列的默认分配策略，继承自 StaticAllocPolicy 并添加 queue_type
 *
 * @tparam ValueType 元素类型
 * @tparam SizeType  索引类型（必须为无符号整型）
 * @tparam Capacity  容量（编译期常量）
 * @tparam QType     堆类型（默认为最大堆）
 * @tparam Alignment 对齐要求（默认 alignof(ValueType)）
 */
template <typename ValueType,
          typename SizeType,
          SizeType Capacity,
          QueueType QType       = QueueType::MaxHeap,
          std::size_t Alignment = alignof(ValueType)>
struct PriorityQueueAllocPolicy : StaticAllocPolicy<ValueType, SizeType, Capacity, Alignment> {
    static constexpr QueueType queue_type = QType; ///< 堆类型，供容器使用
};

namespace detail
{
/**
 * @brief 默认优先级队列分配策略的辅助别名（使用 std::size_t 作为索引类型）
 * @tparam ValueType 元素类型
 * @tparam Capacity  容量
 * @tparam QType     堆类型（默认为最大堆）
 */
template <typename ValueType, std::uint32_t Capacity, QueueType QType = QueueType::MaxHeap>
using DefaultPriorityQueueAllocPolicy_ = PriorityQueueAllocPolicy<ValueType, std::uint32_t, Capacity, QType>;
} // namespace detail

/**
 * @brief 静态优先级队列容器
 *
 * @tparam Allocpolicy  分配策略，必须提供 value_type、size_type、capacity 和内嵌 queue_type
 * @tparam ErrorHandler 错误处理器类型，默认 FatalErrorHandler 并关联内核 ID base_priority_queue_handler_id
 */
template <typename Allocpolicy,
          typename ErrorHandler = FatalErrorHandler<kernel_handler_id::base_priority_queue_handler_id>,
          typename =
              std::enable_if_t<mu_sstl::is_valid_error_handler_v<ErrorHandler> &&
                               detail::has_queue_type_v<Allocpolicy> && detail::is_valid_queue_type_v<Allocpolicy>>>
class PriorityQueue {
    static_assert(detail::has_queue_type_v<Allocpolicy> && detail::is_valid_queue_type_v<Allocpolicy>,
                  "Allocpolicy must have a valid queue_type member of type QueueType");

  public:
    /** @brief 底层存储类型，基于 StaticArray */
    using Storage = StaticArray<Allocpolicy>;
    /** @brief 比较器类型，根据 Allocpolicy::queue_type 推导 */
    using Compare = detail::HeapCompare_<Allocpolicy::queue_type>;

    // 类型别名（符合 STL 规范）
    using value_type                    = typename Storage::value_type;      ///< 元素类型
    using size_type                     = typename Storage::size_type;       ///< 大小类型
    using difference_type               = typename Storage::difference_type; ///< 差值类型
    using reference                     = value_type&;                       ///< 引用
    using const_reference               = const value_type&;                 ///< 常引用
    using pointer                       = value_type*;                       ///< 指针
    using const_pointer                 = const value_type*;                 ///< 常指针

    static constexpr size_type capacity = Storage::capacity; ///< 队列容量（编译期常量）

    static_assert(capacity > 0, "PriorityQueue capacity must be positive");
    static_assert(mu_sstl::is_valid_error_handler_v<ErrorHandler>,
                  "ErrorHandler must have is_fatal, id, and a const call operator() that never returns");
    static_assert(capacity <= (size_type(-1) - 2) / 2,
                  "capacity too large, may cause overflow in child index calculation");

    // ----- 构造 / 析构 -----
    /** @brief 默认构造函数，所有元素零初始化，队列为空 */
    PriorityQueue() = default;

    /**
     * @brief 构造一个带有自定义错误处理器的优先级队列
     * @tparam EH 错误处理器类型（必须可转换为 ErrorHandler）
     * @param eh 错误处理器实例（完美转发）
     */
    template <typename EH>
    explicit PriorityQueue(EH&& eh)
        : errorHandler_(std::forward<EH>(eh)) {}

    // 禁止拷贝/移动
    PriorityQueue(const PriorityQueue&)            = delete;
    PriorityQueue& operator=(const PriorityQueue&) = delete;
    PriorityQueue(PriorityQueue&&)                 = delete;
    PriorityQueue& operator=(PriorityQueue&&)      = delete;

    // ----- 容量查询 -----
    /** @brief 返回当前元素个数 */
    [[nodiscard]] inline constexpr size_type size() const noexcept {
        return size_;
    }

    /** @brief 返回最大容量（始终等于 capacity） */
    [[nodiscard]] inline constexpr size_type max_size() const noexcept {
        return capacity;
    }

    /** @brief 检查队列是否为空 */
    [[nodiscard]] inline constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    /** @brief 检查队列是否已满 */
    [[nodiscard]] inline constexpr bool full() const noexcept {
        return size_ == capacity;
    }

    /** @brief 返回剩余可用空间（可插入的元素个数） */
    [[nodiscard]] inline constexpr size_type available() const noexcept {
        return capacity - size_;
    }

    // ----- 元素访问 -----
    /**
     * @brief 访问顶端元素（优先级最高/最低的元素）
     * @return 顶端元素的引用
     * @note 若队列为空，调用错误处理器（永不返回）
     */
    [[nodiscard]] inline constexpr reference top() {
        if (empty()) errorHandler_(ErrorContext{ErrorCode::Empty, static_cast<std::uint32_t>(0x00)});
        return data_[0];
    }
    /** @copydoc top() */
    [[nodiscard]] inline constexpr const_reference top() const {
        if (empty()) errorHandler_(ErrorContext{ErrorCode::Empty, static_cast<std::uint32_t>(0x00)});
        return data_[0];
    }

    // ----- 插入 -----
    /**
     * @brief 插入一个元素
     * @param value 要插入的值
     * @note 若队列已满，调用错误处理器（永不返回）
     */
    inline constexpr void push(const value_type& value) {
        if (full()) errorHandler_(ErrorContext{ErrorCode::Full, static_cast<std::uint32_t>(0x01)});
        data_[size_] = value;
        sift_up_(size_);
        ++size_;
    }

    // ----- 删除 -----
    /**
     * @brief 删除顶端元素
     * @note 若队列为空，调用错误处理器（永不返回）
     */
    inline constexpr void pop() {
        if (empty()) errorHandler_(ErrorContext{ErrorCode::Empty, static_cast<std::uint32_t>(0x00)});
        data_[0] = data_[size_ - 1];
        --size_;
        if (size_ > 0) sift_down_(0);
    }

    // ----- 清空 -----
    /** @brief 清空队列，所有元素被丢弃 */
    inline constexpr void clear() noexcept {
        size_ = 0;
    }

    // ----- 交换操作 -----
    /**
     * @brief 交换两个优先级队列的内容（不交换错误处理器）
     * @param other 另一个 PriorityQueue 对象
     */
    inline constexpr void swap(PriorityQueue& other) noexcept {
        using std::swap;
        swap(data_, other.data_);
        swap(size_, other.size_);
        // 不交换 errorHandler_ 和 comp_（comp_ 为空类，交换与否无影响）
    }

  private:
    Storage data_{};              ///< 底层静态数组存储
    Compare comp_{};              ///< 比较器对象（空类）
    size_type size_{};            ///< 当前元素个数
    ErrorHandler errorHandler_{}; ///< 错误处理器实例

    /**
     * @brief 向上调整堆，用于插入后恢复堆性质
     * @param idx 起始索引（新插入元素的位置）
     */
    inline constexpr void sift_up_(size_type idx) noexcept {
        using std::swap;
        while (idx > 0) {
            size_type parent = (idx - 1) / 2;
            // 若当前节点与父节点已满足堆序，则停止
            if (!comp_(data_[parent], data_[idx])) break;
            swap(data_[parent], data_[idx]);
            idx = parent;
        }
    }

    /**
     * @brief 向下调整堆，用于删除顶端后恢复堆性质
     * @param idx 起始索引（通常为 0）
     */
    inline constexpr void sift_down_(size_type idx) noexcept {
        using std::swap;
        while (true) {
            size_type left   = 2 * idx + 1;
            size_type right  = 2 * idx + 2;
            size_type target = idx;
            if (left < size_ && comp_(data_[target], data_[left])) target = left;
            if (right < size_ && comp_(data_[target], data_[right])) target = right;
            if (target == idx) break;
            swap(data_[idx], data_[target]);
            idx = target;
        }
    }
};

// ----- 非成员 swap 重载 -----
/**
 * @brief 交换两个 PriorityQueue 对象的内容（ADL 支持）
 * @relates PriorityQueue
 */
template <typename Allocpolicy, typename ErrorHandler>
inline constexpr void swap(PriorityQueue<Allocpolicy, ErrorHandler>& a,
                           PriorityQueue<Allocpolicy, ErrorHandler>& b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

/**
 * @brief 优先级队列的默认类型别名，使用 std::size_t 作为索引类型
 * @tparam ValueType 元素类型
 * @tparam Capacity  容量
 * @tparam QType     堆类型（默认为最大堆）
 */
template <typename ValueType, std::uint32_t Capacity, QueueType QType = QueueType::MaxHeap>
using DefaultPriorityQueue = PriorityQueue<detail::DefaultPriorityQueueAllocPolicy_<ValueType, Capacity, QType>>;

} // namespace mu_sstl

#endif // MU_SSTL_PRIORITY_QUEUE_HPP