/**
 * @file intrusive_list.hpp
 * @author YT_Minro (yurilt15312@outlook.com)
 * @brief 静态侵入式双向链表容器，适用于嵌入式高安全环境
 * @version 1.0.0
 * @date 2026-03-15
 *
 * @copyright Copyright (c) 2026
 *
 * @details
 * 提供编译期大小固定的侵入式双向链表，基于 StaticArray 实现节点池管理。
 * 所有节点预先分配在静态数组中，节点内嵌数据及前后向索引，通过空闲链表
 * 实现 O(1) 的节点分配与释放。链表操作不涉及动态内存，行为可预测。
 *
 * 主要特性：
 * - 固定容量，无动态内存分配，行为可预测
 * - 模板参数 AllocPolicy 指定节点类型、索引类型和容量，节点必须包含 data、prev、next 成员
 * - 可定制的错误处理器（空链表访问、节点分配失败时调用），默认处理器使用 FatalErrorHandler 并关联内核 ID
 * - 严格的拷贝/移动控制（禁止拷贝和移动，确保唯一所有权）
 * - 零初始化保证（默认构造所有节点为零，空闲链表正确建立）
 * - 提供前后插入、指定位置插入、删除、查找、遍历等标准双向链表操作
 * - 支持通过回调函数安全遍历（避免迭代器生命周期问题）
 *
 * 该容器专为裸机嵌入式 RTOS 设计，无异常、无动态内存分配，
 * 所有不调用错误处理器的操作均为 noexcept。
 *
 * @par 使用示例
 * @code
 * #include "mu_sstl/containers/intrusive_list.hpp"
 *
 * // 定义节点类型（需包含 data、prev、next，其中 prev/next 类型应与索引类型一致）
 * struct TaskNode {
 *     int data;
 *     std::size_t prev;
 *     std::size_t next;
 * };
 *
 * // 定义分配策略：节点类型为 TaskNode，索引类型为 std::size_t，容量为 10
 * using MyPolicy = mu_sstl::IntrusiveListAllocPolicy<TaskNode, std::size_t, 10>;
 * // 定义链表类型，使用默认错误处理器
 * using MyList = mu_sstl::IntrusiveList<MyPolicy>;
 *
 * MyList list;
 *
 * // 插入元素
 * list.push_front(10);
 * list.push_back(20);
 *
 * // 访问首尾
 * int front = list.front();  // 10
 * int back  = list.back();   // 20
 *
 * // 遍历
 * list.for_each([](const int& val) { process(val); });
 *
 * // 查找
 * auto idx = list.find(20);  // 返回节点索引（非连续）
 *
 * // 在指定位置后插入
 * if (idx != MyList::npos) {
 *     list.insert_after(idx, 25);
 * }
 *
 * // 删除节点
 * list.pop_front();
 *
 * // 自定义错误处理器（必须永不返回）
 * struct MyErrorHandler {
 *     static constexpr bool is_fatal = true;
 *     static constexpr std::uint32_t id = 0x200;
 *     [[noreturn]] void operator()(const mu_sstl::ErrorContext& ctx) const {
 *         // 记录错误、复位系统或进入死循环
 *         while (true) {}
 *     }
 * };
 * mu_sstl::IntrusiveList<MyPolicy, MyErrorHandler> safe_list;
 * @endcode
 *
 * @warning 自定义 ErrorHandler 必须满足 is_valid_error_handler_v 的要求：
 *          - 提供静态成员 is_fatal (bool) 和 id (可转为 uint16_t)
 *          - 提供 const 限定的 operator()(const ErrorContext&) 且永不返回
 *          否则在空链表访问、分配失败等错误时将产生未定义行为。
 * @note 该类禁止拷贝和移动，因为底层存储不可复制（StaticArray 已禁用拷贝/移动）。
 * @note 插入和删除操作中，传入的 pos 必须为当前有效链表中存在的节点索引，
 *       否则行为未定义。在性能敏感的路径中，调用者需自行保证有效性。
 */
#pragma once

#ifndef MU_SSTL_INTRUSIVE_LIST_HPP
#define MU_SSTL_INTRUSIVE_LIST_HPP

#include "mu_sstl/containers/static_array.hpp"
#include "mu_sstl/errors/basic_error.hpp"
#include "mu_sstl/errors/kernel_handler_id.hpp"
#include <cstddef>     // for std::size_t
#include <cstdint>     // for uint32_t
#include <type_traits> // for std::enable_if_t, std::is_unsigned_v

namespace mu_sstl
{
namespace detail
{
/** @brief 检测类型 T 是否包含名为 data 的成员 */
template <typename T, typename = void>
struct has_data_member : std::false_type {};
template <typename T>
struct has_data_member<T, std::void_t<decltype(std::declval<T>().data)>> : std::true_type {};

/** @brief 检测类型 T 是否包含名为 prev 的成员 */
template <typename T, typename = void>
struct has_prev_member : std::false_type {};
template <typename T>
struct has_prev_member<T, std::void_t<decltype(std::declval<T>().prev)>> : std::true_type {};

/** @brief 检测类型 T 是否包含名为 next 的成员 */
template <typename T, typename = void>
struct has_next_member : std::false_type {};
template <typename T>
struct has_next_member<T, std::void_t<decltype(std::declval<T>().next)>> : std::true_type {};

/** @brief 判断 T 是否为有效的节点类型（必须包含 data, prev, next 成员） */
template <typename T>
using is_valid_node_type = std::conjunction<has_data_member<T>, has_prev_member<T>, has_next_member<T>>;

template <typename T>
constexpr bool is_valid_node_type_v = is_valid_node_type<T>::value;
template <typename T>
constexpr bool has_prev_member_v = has_prev_member<T>::value;
template <typename T>
constexpr bool has_next_member_v = has_next_member<T>::value;
template <typename T>
constexpr bool has_data_member_v = has_data_member<T>::value;

/**
 * @brief 侵入式链表的默认节点结构
 * @tparam T        用户数据类型
 * @tparam SizeType 索引类型（必须为无符号整型）
 */
template <typename T, typename SizeType, typename = std::enable_if_t<std::is_unsigned_v<SizeType>>>
struct Node_ {
    using size_type  = SizeType; ///< 索引类型
    using value_type = T;        ///< 用户数据类型

    T data;           ///< 用户数据
    size_type prev{}; ///< 前驱节点索引
    size_type next{}; ///< 后继节点索引
};
} // namespace detail

/**
 * @brief 侵入式链表的分配策略辅助别名
 * @tparam ValueType 用户数据类型
 * @tparam SizeType  索引类型（必须为无符号整型）
 * @tparam Capacity  链表容量（编译期常量）
 */
template <typename ValueType, typename SizeType, SizeType Capacity>
using IntrusiveListAllocPolicy = StaticAllocPolicy<detail::Node_<ValueType, SizeType>, SizeType, Capacity>;

/**
 * @brief 静态侵入式双向链表容器
 *
 * @tparam AllocPolicy  分配策略，必须提供 value_type（节点类型）、size_type、capacity
 * @tparam ErrorHandler 错误处理器类型，默认 FatalErrorHandler 并关联内核 ID base_intrusive_list_handler_id
 */
template <typename AllocPolicy,
          typename ErrorHandler = FatalErrorHandler<kernel_handler_id::base_intrusive_list_handler_id>,
          typename              = std::enable_if_t<mu_sstl::is_valid_error_handler_v<ErrorHandler> &&
                                                   detail::has_size_type_v<typename AllocPolicy::value_type> &&
                                                   detail::has_value_type_v<typename AllocPolicy::value_type> &&
                                                   detail::is_valid_node_type_v<typename AllocPolicy::value_type>>>
class IntrusiveList {
    // ----- 编译期约束验证 -----
    static_assert(detail::has_size_type_v<typename AllocPolicy::value_type>,
                  "size_type must be defined in value_type of AllocPolicy.");
    static_assert(detail::has_value_type_v<typename AllocPolicy::value_type>,
                  "value_type must be defined in value_type of AllocPolicy.");
    static_assert(detail::has_prev_member_v<typename AllocPolicy::value_type>,
                  "value_type must have a member named 'prev'.");
    static_assert(detail::has_next_member_v<typename AllocPolicy::value_type>,
                  "value_type must have a member named 'next'.");
    static_assert(detail::has_data_member_v<typename AllocPolicy::value_type>,
                  "value_type must have a member named 'data'.");

  public:
    using Pool                          = StaticArray<AllocPolicy>;       ///< 节点池（静态数组）
    using node_type                     = typename Pool::value_type;      ///< 节点类型
    using value_type                    = typename node_type::value_type; ///< 用户数据类型
    using size_type                     = typename Pool::size_type;       ///< 索引类型
    using difference_type               = typename Pool::difference_type; ///< 差值类型
    using reference                     = value_type&;                    ///< 引用
    using const_reference               = const value_type&;              ///< 常引用
    using pointer                       = value_type*;                    ///< 指针
    using const_pointer                 = const value_type*;              ///< 常指针

    static constexpr size_type capacity = Pool::capacity;             ///< 链表容量
    static constexpr size_type npos     = static_cast<size_type>(-1); ///< 无效索引

    static_assert(capacity > 0, "IntrusiveList capacity must be positive");
    static_assert(mu_sstl::is_valid_error_handler_v<ErrorHandler>,
                  "ErrorHandler must have is_fatal, id, and a const call operator() that never returns");
    static_assert(std::is_object_v<value_type> && !std::is_abstract_v<value_type>,
                  "value_type must be a concrete (non-abstract) object type");
    // 检查 value_type 是否完整（防止前向声明类型）
    static_assert(static_cast<std::size_t>(sizeof(value_type)) > 0, "value_type must be a complete type");

    // 检查节点成员类型与容器期望的类型一致
    static_assert(std::is_same_v<decltype(std::declval<node_type>().prev), size_type>,
                  "prev member must be of type size_type");
    static_assert(std::is_same_v<decltype(std::declval<node_type>().next), size_type>,
                  "next member must be of type size_type");
    static_assert(std::is_same_v<decltype(std::declval<node_type>().data), value_type>,
                  "data member must be of type value_type");

    /**
     * @brief 默认构造函数，初始化空闲链表并将有效链表置空
     */
    IntrusiveList() {
        init_free_list_();
    }

    /**
     * @brief 构造一个带有自定义错误处理器的链表
     * @tparam EH 错误处理器类型（必须可转换为 ErrorHandler）
     * @param eh 错误处理器实例（完美转发）
     */
    template <typename EH = ErrorHandler>
    explicit IntrusiveList(EH&& eh)
        : errorHandler_(std::forward<EH>(eh)) {
        init_free_list_();
    }

    // 禁用拷贝/移动
    IntrusiveList(const IntrusiveList&)            = delete;
    IntrusiveList& operator=(const IntrusiveList&) = delete;
    IntrusiveList(IntrusiveList&&)                 = delete;
    IntrusiveList& operator=(IntrusiveList&&)      = delete;

    // ----- 容量查询 -----
    /** @brief 检查链表是否为空 */
    [[nodiscard]] inline constexpr bool empty() const noexcept {
        return count_ == 0;
    }

    /** @brief 检查链表是否已满 */
    [[nodiscard]] inline constexpr bool full() const noexcept {
        return count_ == capacity;
    }

    /** @brief 返回当前元素个数 */
    [[nodiscard]] inline constexpr size_type size() const noexcept {
        return count_;
    }

    /** @brief 返回链表最大容量（编译期常量） */
    [[nodiscard]] inline constexpr size_type max_size() const noexcept {
        return capacity;
    }

    /** @brief 返回剩余可用节点数（可插入的元素个数） */
    [[nodiscard]] inline constexpr size_type available() const noexcept {
        return capacity - count_;
    }

    // ----- 元素访问 -----
    /** @brief 访问第一个元素（链表为空时调用错误处理器） */
    [[nodiscard]] inline constexpr reference front() {
        if (empty()) errorHandler_(ErrorContext{ErrorCode::Empty, static_cast<std::uint32_t>(0x00)});
        return pool_[head_].data;
    }
    /** @brief 访问第一个元素（常版本） */
    [[nodiscard]] inline constexpr const_reference front() const {
        if (empty()) errorHandler_(ErrorContext{ErrorCode::Empty, static_cast<std::uint32_t>(0x00)});
        return pool_[head_].data;
    }

    /** @brief 访问最后一个元素（链表为空时调用错误处理器） */
    [[nodiscard]] inline constexpr reference back() {
        if (empty()) errorHandler_(ErrorContext{ErrorCode::Empty, static_cast<std::uint32_t>(0x00)});
        return pool_[tail_].data;
    }
    /** @brief 访问最后一个元素（常版本） */
    [[nodiscard]] inline constexpr const_reference back() const {
        if (empty()) errorHandler_(ErrorContext{ErrorCode::Empty, static_cast<std::uint32_t>(0x00)});
        return pool_[tail_].data;
    }

    // ----- 插入操作 -----
    /**
     * @brief 在链表头部插入元素
     * @param value 要插入的值
     * @note 若节点池已满，调用错误处理器
     */
    inline constexpr void push_front(const value_type& value) {
        size_type new_node   = allocate_node_();
        pool_[new_node].data = value;
        pool_[new_node].prev = npos;
        pool_[new_node].next = head_;

        if (head_ != npos) {
            pool_[head_].prev = new_node;
        } else {
            tail_ = new_node; // 原为空，更新尾指针
        }
        head_ = new_node;
        ++count_;
    }

    /**
     * @brief 在链表尾部插入元素
     * @param value 要插入的值
     * @note 若节点池已满，调用错误处理器
     */
    inline constexpr void push_back(const value_type& value) {
        size_type new_node   = allocate_node_();
        pool_[new_node].data = value;
        pool_[new_node].next = npos;
        pool_[new_node].prev = tail_;

        if (tail_ != npos) {
            pool_[tail_].next = new_node;
        } else {
            head_ = new_node; // 原为空，更新头指针
        }
        tail_ = new_node;
        ++count_;
    }

    /**
     * @brief 在指定节点之后插入元素
     * @param pos   目标节点索引（必须属于当前有效链表）
     * @param value 要插入的值
     * @note 若 pos 无效（npos）或链表为空，调用错误处理器。
     *       若节点池已满，调用错误处理器。
     *       调用者需保证 pos 是有效节点索引，否则行为未定义。
     */
    inline constexpr void insert_after(size_type pos, const value_type& value) {
        if (pos == npos || count_ == 0)
            errorHandler_(ErrorContext{ErrorCode::InvalidArgument, static_cast<std::uint32_t>(0x00)});
        // 可选：进一步验证 pos 是否属于当前链表（可通过遍历或维护标志，但为性能可不做，信任用户）
        size_type new_node   = allocate_node_();
        pool_[new_node].data = value;
        pool_[new_node].prev = pos;
        pool_[new_node].next = pool_[pos].next;

        if (pool_[pos].next != npos) {
            pool_[pool_[pos].next].prev = new_node;
        } else {
            tail_ = new_node; // 在尾部插入，更新尾指针
        }
        pool_[pos].next = new_node;
        ++count_;
    }

    /**
     * @brief 在指定节点之前插入元素
     * @param pos   目标节点索引（必须属于当前有效链表）
     * @param value 要插入的值
     * @note 若 pos 无效（npos）或链表为空，调用错误处理器。
     *       若节点池已满，调用错误处理器。
     *       调用者需保证 pos 是有效节点索引，否则行为未定义。
     */
    inline constexpr void insert_before(size_type pos, const value_type& value) {
        if (pos == npos || count_ == 0)
            errorHandler_(ErrorContext{ErrorCode::InvalidArgument, static_cast<std::uint32_t>(0x00)});
        size_type new_node   = allocate_node_();
        pool_[new_node].data = value;
        pool_[new_node].prev = pool_[pos].prev;
        pool_[new_node].next = pos;

        if (pool_[pos].prev != npos) {
            pool_[pool_[pos].prev].next = new_node;
        } else {
            head_ = new_node; // 在头部之前插入，更新头指针
        }
        pool_[pos].prev = new_node;
        ++count_;
    }

    // ----- 删除操作 -----
    /** @brief 删除头节点（链表为空时调用错误处理器） */
    inline constexpr void pop_front() {
        if (empty()) errorHandler_(ErrorContext{ErrorCode::Empty, static_cast<std::uint32_t>(0x00)});
        size_type old_head = head_;
        head_              = pool_[head_].next;
        if (head_ != npos) {
            pool_[head_].prev = npos;
        } else {
            tail_ = npos; // 链表变空
        }
        deallocate_node_(old_head);
        --count_;
    }

    /** @brief 删除尾节点（链表为空时调用错误处理器） */
    inline constexpr void pop_back() {
        if (empty()) errorHandler_(ErrorContext{ErrorCode::Empty, static_cast<std::uint32_t>(0x00)});
        size_type old_tail = tail_;
        tail_              = pool_[tail_].prev;
        if (tail_ != npos) {
            pool_[tail_].next = npos;
        } else {
            head_ = npos; // 链表变空
        }
        deallocate_node_(old_tail);
        --count_;
    }

    /**
     * @brief 删除指定节点
     * @param pos 要删除的节点索引（必须属于当前有效链表）
     * @note 若 pos 为 npos 或链表为空，调用错误处理器。
     *       调用者需保证 pos 是有效节点索引，否则行为未定义。
     */
    inline constexpr void erase(size_type pos) {
        if (pos == npos || empty())
            errorHandler_(ErrorContext{ErrorCode::InvalidArgument, static_cast<std::uint32_t>(0x00)});
        // 更新前后节点链接
        if (pool_[pos].prev != npos) {
            pool_[pool_[pos].prev].next = pool_[pos].next;
        } else {
            head_ = pool_[pos].next; // 删除的是头节点
        }
        if (pool_[pos].next != npos) {
            pool_[pool_[pos].next].prev = pool_[pos].prev;
        } else {
            tail_ = pool_[pos].prev; // 删除的是尾节点
        }
        deallocate_node_(pos);
        --count_;
    }

    /** @brief 清空链表，将所有节点归还空闲链表 */
    inline constexpr void clear() noexcept {
        // 将所有有效节点归还空闲链表
        while (!empty()) {
            pop_front(); // pop_front 内部已更新 count_
        }
        // 此时 head_ = tail_ = npos, count_ = 0
    }

    // ----- 查找与遍历 -----
    /**
     * @brief 查找第一个值与给定值相等的节点
     * @param value 要查找的值
     * @return 节点索引，若未找到返回 npos
     */
    [[nodiscard]] inline constexpr size_type find(const value_type& value) const noexcept {
        size_type idx = head_;
        while (idx != npos) {
            if (pool_[idx].data == value) return idx;
            idx = pool_[idx].next;
        }
        return npos;
    }

    /**
     * @brief 对链表每个元素执行回调函数（安全遍历，避免迭代器失效问题）
     * @tparam Func 可调用对象类型（接受 const_reference 参数）
     * @param f     回调函数
     */
    template <typename Func>
    void for_each(Func f) const {
        size_type idx = head_;
        while (idx != npos) {
            f(pool_[idx].data);
            idx = pool_[idx].next;
        }
    }

    /**
     * @brief 对链表每个元素执行回调函数（安全遍历，避免迭代器失效问题）
     * @tparam Func 可调用对象类型（接受 const_reference 参数）
     * @param f     回调函数
     */
    template <typename Func>
    void for_each(Func f) {
        size_type idx = head_;
        while (idx != npos) {
            f(pool_[idx].data);
            idx = pool_[idx].next;
        }
    }

    // ----- 交换操作 -----
    /**
     * @brief 交换两个链表的内容（不交换错误处理器）
     * @param other 另一个 IntrusiveList 对象
     */
    inline constexpr void swap(IntrusiveList& other) noexcept {
        using std::swap;
        swap(pool_, other.pool_);
        swap(head_, other.head_);
        swap(tail_, other.tail_);
        swap(free_head_, other.free_head_);
        swap(count_, other.count_);
        // 不交换 errorHandler_
    }

  private:
    Pool pool_{};                 ///< 节点池
    size_type head_{npos};        ///< 头节点索引
    size_type tail_{npos};        ///< 尾节点索引
    size_type free_head_{};       ///< 空闲节点链表头索引
    size_type count_{};           ///< 当前元素个数
    ErrorHandler errorHandler_{}; ///< 错误处理器实例

    /** @brief 初始化空闲链表（将所有节点通过 next 链接成单向链表） */
    inline void init_free_list_() {
        // 将节点池中所有节点通过 next 链接成单向空闲链表
        for (size_type i = 0; i < capacity - 1; ++i) {
            pool_[i].next = i + 1;
        }
        pool_[capacity - 1].next = npos;
        head_                    = npos;
        tail_                    = npos;
        free_head_               = 0;
        count_                   = 0;
    }

    /**
     * @brief 分配一个节点，返回其索引
     * @return 节点索引
     * @note 如果没有可用节点，调用错误处理器（永不返回）
     */
    [[nodiscard]] inline constexpr size_type allocate_node_() {
        if (free_head_ == npos) {
            errorHandler_(ErrorContext{ErrorCode::AllocationFailed, static_cast<uint32_t>(npos)});
        }
        size_type index = free_head_;
        free_head_      = pool_[index].next;
        return index;
    }

    /**
     * @brief 将节点归还空闲链表
     * @param index 要释放的节点索引
     * @note 此方法为 unsafe 版本，调用者必须保证 index 有效（属于已分配的节点），
     *       通常在性能敏感且已验证索引的场景使用，否则行为未定义。
     */
    inline constexpr void deallocate_node_(size_type index) noexcept {
        pool_[index].next = free_head_;
        free_head_        = index;
    }
};

/**
 * @brief 非成员 swap 重载，支持 ADL 查找
 * @relates IntrusiveList
 */
template <typename AllocPolicy, typename ErrorHandler>
inline constexpr void swap(IntrusiveList<AllocPolicy, ErrorHandler>& a,
                           IntrusiveList<AllocPolicy, ErrorHandler>& b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

/**
 * @brief IntrusiveList 的默认类型别名，使用 std::size_t 作为索引类型
 * @tparam ValueType 用户数据类型
 * @tparam SizeType  索引类型（必须为无符号整型）
 * @tparam Capacity  链表容量（编译期常量）
 */
template <typename ValueType, typename SizeType, SizeType Capacity>
using DefaultIntrusiveList = IntrusiveList<IntrusiveListAllocPolicy<ValueType, SizeType, Capacity>>;

} // namespace mu_sstl

#endif // MU_SSTL_INTRUSIVE_LIST_HPP