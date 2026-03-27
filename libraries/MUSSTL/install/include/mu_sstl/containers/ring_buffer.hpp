/**
 * @file RingBuffer.hpp
 * @author YT_Minro (yurilt15312@outlook.com)
 * @brief 静态环形缓冲区，适用于嵌入式高安全环境
 * @version 1.0.0
 * @date 2026-03-13
 *
 * @copyright Copyright (c) 2026
 *
 * @details
 * 提供编译期大小固定的环形缓冲区容器，基于 StaticArray 实现。
 * 支持 FIFO（先进先出）操作，并针对嵌入式环境进行了安全优化：
 * - 固定容量，无动态内存分配，行为可预测
 * - 模板参数 T 指定元素类型，N 指定容量（编译期常量）
 * - 可定制的错误处理器（满/空时调用），默认处理器进入死循环
 * - 严格的拷贝/移动控制（禁止拷贝和移动，确保唯一所有权）
 * - 零初始化保证（默认构造所有元素清零）
 * - 提供带安全检查的 front/back/push/pop 以及无异常的 try_* 版本
 * - 提供容量查询（size、max_size、empty、full、available）
 *
 * 该容器专为裸机嵌入式 RTOS 设计，无异常、无动态内存分配，
 * 所有不调用错误处理器的操作均为 noexcept。
 *
 * @par 使用示例
 * @code
 * #include "mu_sstl/containers/RingBuffer.hpp"
 *
 * // 创建一个可存储 16 个 int 的环形缓冲区，使用默认错误处理器（死循环）
 * mu_sstl::RingBuffer<int, 16> rb;
 *
 * // 推入元素
 * rb.push(42);
 * rb.push(100);
 *
 * // 查看队首元素
 * int x = rb.front();  // x = 42
 *
 * // 弹出元素（带安全检查）
 * int y;
 * if (rb.try_pop(y)) {
 *     // 成功弹出，y = 42
 * }
 *
 * // 清空缓冲区
 * rb.clear();
 *
 * // 自定义错误处理器（必须永不返回）
 * struct MyErrorHandler {
 *     [[noreturn]] void operator()() const {
 *         // 记录错误、复位系统或进入死循环
 *         while (true) {}
 *     }
 * };
 * mu_sstl::RingBuffer<char, 32, MyErrorHandler> safe_rb;
 * @endcode
 *
 * @warning 自定义 ErrorHandler 必须保证 operator() 永不返回，否则调用其的成员函数（如 front()、push()）
 *          在满/空时将产生未定义行为。
 * @warning 当缓冲区满时调用 push() 会触发错误处理器，当缓冲区空时调用 front()、back()、pop()
 *          也会触发错误处理器。建议在不确定状态下使用 try_* 版本进行安全操作。
 * @note 该类禁止拷贝和移动，因为底层存储不可复制（StaticArray 已禁用拷贝/移动）。
 */

#pragma once

#ifndef MU_SSTL_RING_BUFFER_HPP
#define MU_SSTL_RING_BUFFER_HPP

#include "mu_sstl/containers/static_array.hpp"
#include "mu_sstl/errors/basic_error.hpp"
#include "mu_sstl/errors/kernel_handler_id.hpp"
#include <cstddef>     // for std::size_t
#include <cstdint>     // for uint32_t
#include <type_traits> // for std::is_invocable_v, std::enable_if_t
#include <utility>     // for std::forward

namespace mu_sstl
{

template <typename ValueType, typename SizeType, SizeType Capacity, std::size_t Alignment = alignof(ValueType)>
using RingBufferAllocPolicy = StaticAllocPolicy<ValueType, SizeType, Capacity, Alignment>;

namespace detail
{
template <typename ValueType, std::uint32_t Capacity>
using DefaultRingbufferAllocPolicy_ = RingBufferAllocPolicy<ValueType, std::size_t, Capacity, platform_max_align_t>;
} // namespace detail

/**
 * @brief 静态环形缓冲区，固定容量，基于 StaticArray 存储
 *
 * @tparam AllocPolicy  分配策略，必须提供 value_type、size_type、capacity、alignment
 * @tparam ErrorHandler 错误处理器类型，默认 DefaultErrorHandler
 *
 * 提供 FIFO 操作，支持满/空检查，错误处理可定制。
 * 所有操作均不涉及动态内存分配，适合嵌入式环境。
 */
template <typename AllocPolicy,
          typename ErrorHandler = FatalErrorHandler<kernel_handler_id::base_ring_buffer_handler_id>,
          typename              = std::enable_if_t<mu_sstl::is_valid_error_handler_v<ErrorHandler>>>
class RingBuffer {

  public:
    using Storage = StaticArray<AllocPolicy>;
    // 类型别名（符合 STL 规范）
    using value_type                    = typename Storage::value_type;
    using size_type                     = typename Storage::size_type;
    using difference_type               = typename Storage::difference_type;
    using reference                     = value_type&;
    using const_reference               = const value_type&;
    using pointer                       = value_type*;
    using const_pointer                 = const value_type*;

    static constexpr size_type capacity = Storage::capacity; ///< 缓冲区容量

    static_assert(capacity > 0, "RingBuffer capacity must be positive");
    static_assert(mu_sstl::is_valid_error_handler_v<ErrorHandler>,
                  "ErrorHandler must have is_fatal, id, and a const call operator() that never returns");
    static_assert(std::is_object_v<value_type> && !std::is_abstract_v<value_type>,
                  "value_type must be a concrete (non-abstract) object type");
    // 检查 value_type 是否完整（防止前向声明类型）
    static_assert(static_cast<std::size_t>(sizeof(value_type)) > 0, "value_type must be a complete type");

    // ----- 构造 / 析构 -----
    RingBuffer() = default;

    template <typename EH, typename = std::enable_if_t<std::is_constructible_v<ErrorHandler, EH&&>>>
    explicit RingBuffer(EH&& eh)
        : errorHandler_(std::forward<EH>(eh)) {}

    // 禁止拷贝/移动（确保唯一所有权）
    RingBuffer(const RingBuffer&)            = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    RingBuffer(RingBuffer&&)                 = delete;
    RingBuffer& operator=(RingBuffer&&)      = delete;

    // ----- 元素访问 -----
    [[nodiscard]] inline reference front() {
        if (empty()) errorHandler_(ErrorContext{ErrorCode::Empty, static_cast<uint32_t>(0x00)});
        return buffer_[head_];
    }
    [[nodiscard]] inline const_reference front() const {
        if (empty()) errorHandler_(ErrorContext{ErrorCode::Empty, static_cast<uint32_t>(0x00)});
        return buffer_[head_];
    }

    [[nodiscard]] inline reference back() {
        if (empty()) errorHandler_(ErrorContext{ErrorCode::Empty, static_cast<uint32_t>(tail_)});
        return buffer_[(tail_ == 0 ? capacity - 1 : tail_ - 1)];
    }
    [[nodiscard]] inline const_reference back() const {
        if (empty()) errorHandler_(ErrorContext{ErrorCode::Empty, static_cast<uint32_t>(tail_)});
        return buffer_[(tail_ == 0 ? capacity - 1 : tail_ - 1)];
    }

    // ----- 容量查询 -----
    [[nodiscard]] inline constexpr size_type size() const noexcept {
        return count_;
    }
    [[nodiscard]] inline constexpr size_type max_size() const noexcept {
        return capacity;
    }
    [[nodiscard]] inline constexpr bool empty() const noexcept {
        return count_ == 0;
    }
    [[nodiscard]] inline constexpr bool full() const noexcept {
        return count_ == capacity;
    }
    [[nodiscard]] inline constexpr size_type available() const noexcept {
        return capacity - count_;
    }

    // ----- 修改器 -----
    /** 推入元素（满时调用错误处理器） */
    inline void push(const value_type& value) {
        if (full()) errorHandler_(ErrorContext{ErrorCode::Full, static_cast<std::uint32_t>(0x02)});
        buffer_[tail_] = value;
        tail_          = (tail_ + 1) % capacity;
        ++count_;
    }

    /** 尝试推入（返回是否成功，不触发错误处理器） */
    [[nodiscard]] inline constexpr bool try_push(const value_type& value) noexcept {
        if (full()) return false;
        buffer_[tail_] = value;
        tail_          = (tail_ + 1) % capacity;
        ++count_;
        return true;
    }

    /** 弹出元素（空时调用错误处理器） */
    inline void pop() {
        if (empty()) errorHandler_(ErrorContext{ErrorCode::Empty, static_cast<std::uint32_t>(0x01)});
        head_ = (head_ + 1) % capacity;
        --count_;
    }

    /** 尝试弹出（返回是否成功，元素通过出参返回） */
    [[nodiscard]] inline constexpr bool try_pop(value_type& out) noexcept {
        if (empty()) return false;
        out   = buffer_[head_];
        head_ = (head_ + 1) % capacity;
        --count_;
        return true;
    }

    /** 清空缓冲区 */
    inline constexpr void clear() noexcept {
        head_  = 0;
        tail_  = 0;
        count_ = 0;
    }

    // ----- 交换支持 -----
    inline constexpr void swap(RingBuffer& other) noexcept {
        using std::swap;
        swap(buffer_, other.buffer_);
        swap(head_, other.head_);
        swap(tail_, other.tail_);
        swap(count_, other.count_);
        // 不交换 errorHandler_（通常相同或无关）
    }

  private:
    Storage buffer_{};  ///< 底层存储，默认构造（元素零初始化）
    size_type head_{};  ///< 读索引
    size_type tail_{};  ///< 写索引
    size_type count_{}; ///< 当前元素个数
    ErrorHandler errorHandler_{};
};

/** @brief 非成员 swap 重载 */
template <typename AllocPolicy, typename ErrorHandler>
inline constexpr void swap(RingBuffer<AllocPolicy, ErrorHandler>& a,
                           RingBuffer<AllocPolicy, ErrorHandler>& b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

/**
 * @brief RingBuffer 的默认类型别名，使用 std::size_t 作为索引类型
 * @tparam ValueType 元素类型
 * @tparam Capacity  容量
 */
template <typename ValueType, std::uint32_t Capacity>
using DefaultRingBuffer = RingBuffer<detail::DefaultRingbufferAllocPolicy_<ValueType, Capacity>,
                                     FatalErrorHandler<kernel_handler_id::base_ring_buffer_handler_id>>;

} // namespace mu_sstl

#endif // MU_SSTL_RING_BUFFER_HPP
