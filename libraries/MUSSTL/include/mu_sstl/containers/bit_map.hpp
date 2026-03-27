/**
 * @file BitMap.hpp
 * @author YT_Minro (yurilt15312@outlook.com)
 * @brief 静态位图容器，适用于嵌入式高安全环境
 * @version 1.0.0
 * @date 2026-03-13
 *
 * @copyright Copyright (c) 2026
 *
 * @details
 * 提供编译期大小固定的位图容器，支持按位操作和查找，基于 StaticArray 实现安全存储。
 * 主要特性：
 * - 固定位数，无动态内存分配，行为可预测
 * - 模板参数 Bits 指定位数（编译期常量）
 * - 可定制的错误处理器（索引越界时调用），默认处理器使用 FatalErrorHandler 并关联内核 ID
 * - 严格的拷贝/移动控制（禁止拷贝和移动，确保唯一所有权）
 * - 零初始化保证（默认构造所有位为 0）
 * - 提供按位设置、清除、翻转、测试，以及批量查询（any/none/count）和查找（首个0/1）
 * - 支持范围设置/清除（含安全与不安全版本）
 *
 * 该容器专为裸机嵌入式 RTOS 设计，无异常、无动态内存分配，
 * 所有不调用错误处理器的操作均为 noexcept。
 *
 * @par 使用示例
 * @code
 * #include "mu_sstl/containers/BitMap.hpp"
 *
 * // 创建一个 16 位的位图，使用默认错误处理器
 * mu_sstl::BitMap<16> bm;
 *
 * // 设置位 3
 * bm.set(3);
 * // 检查位 3
 * if (bm.test(3)) { ... }
 *
 * // 设置一段范围
 * bm.set_range(5, 10);  // 将第5到10位置1
 *
 * // 不安全版本（更快，但调用者需保证索引有效）
 * bm.unsafe_set_range(0, 15);
 *
 * // 查找第一个空闲位（为0的位）
 * auto idx = bm.find_first_zero();  // 返回 npos 因为全1
 *
 * // 自定义错误处理器（必须永不返回）
 * struct MyErrorHandler {
 *     static constexpr bool is_fatal = true;
 *     static constexpr std::uint32_t id = 0x100;
 *     [[noreturn]] void operator()(mu_sstl::ErrorContext& ctx) const {
 *         // 记录错误、复位系统或进入死循环
 *         while (true) {}
 *     }
 * };
 * mu_sstl::BitMap<32, std::uint32_t, MyErrorHandler> safe_bm;
 * @endcode
 *
 * @warning 自定义 ErrorHandler 必须满足 is_valid_error_handler_v 的要求：
 *          - 提供静态成员 is_fatal (bool) 和 id (可转为 uint16_t)
 *          - 提供 const 限定的 operator()(ErrorContext) 且永不返回
 *          否则在索引越界时将产生未定义行为。
 * @note 该类禁止拷贝和移动，因为底层存储不可复制（StaticArray 已禁用拷贝/移动）。
 */

#pragma once

#ifndef MU_SSTL_BITMAP_HPP
#define MU_SSTL_BITMAP_HPP

#include "mu_sstl/containers/static_array.hpp"
#include "mu_sstl/errors/basic_error.hpp"
#include "mu_sstl/errors/kernel_handler_id.hpp"
#include <climits>     ///< for CHAR_BIT
#include <cstddef>     ///< for std::size_t
#include <cstdint>     ///< for uint8_t, uint32_t
#include <type_traits> ///< for std::enable_if_t, std::is_unsigned_v

namespace mu_sstl
{
namespace detail
{
/** @brief 检测类型 T 是否包含成员bits */
template <typename T, typename = void>
struct has_bits_member : std::false_type {};
template <typename T>
struct has_bits_member<T, std::void_t<decltype(std::declval<T>().bits)>> : std::true_type {};
template <typename T>
constexpr bool has_bits_member_v = has_bits_member<T>::value;
} // namespace detail

/**
 * @brief 为 BitMap 生成 StaticArray 分配策略
 * @tparam BlockType 块类型（无符号整数）
 * @tparam Bits      总位数
 */
template <typename BlockType, typename SizeType, SizeType Bits>
struct BitMapAllocPolicy {
    using value_type                       = BlockType;
    using size_type                        = SizeType;
    static constexpr size_type capacity    = (Bits + sizeof(BlockType) * CHAR_BIT - 1) / (sizeof(BlockType) * CHAR_BIT);
    static constexpr std::size_t alignment = alignof(BlockType);
    static constexpr size_type bits        = Bits;
};

namespace detail
{
template <typename BlockType, std::size_t Bits>
using DefaultBitMapAllocPolicy_ = BitMapAllocPolicy<BlockType, std::size_t, Bits>;
}

/**
 * @brief 静态位图容器，固定位数，基于 StaticArray 存储
 *
 * @tparam AllocPolicy 分配策略类型，必须提供 value_type、size_type、capacity、alignment
 * @tparam ErrorHandler 错误处理器类型，默认 FatalErrorHandler 并关联内核 ID base_bitmap_handler_id
 *
 * 提供位级的操作，所有索引均从 0 开始。当索引越界时，调用错误处理器。
 * 查找操作（find_first_zero/find_first_set）在无结果时返回 npos。
 *
 * @note 错误处理器通过 ErrorContext 传递错误码和位置信息。
 * @warning 索引有效性由调用者保证，除非使用 at() 风格的安全版本（这里通过每个函数内部检查实现）。
 */
template <typename AllocPolicy,
          typename ErrorHandler = FatalErrorHandler<kernel_handler_id::base_bitmap_handler_id>,
          typename = std::enable_if_t<is_valid_error_handler_v<ErrorHandler> && detail::has_value_type_v<AllocPolicy> &&
                                      std::is_unsigned_v<typename AllocPolicy::value_type> &&
                                      detail::has_bits_member_v<AllocPolicy>>>
class BitMap {

  public:
    using Storage                             = StaticArray<AllocPolicy>;
    using size_type                           = typename Storage::size_type;
    using block_type                          = typename Storage::value_type;
    using value_type                          = bool;                                         ///< 逻辑元素类型（位值）
    using difference_type                     = std::ptrdiff_t;                               ///< 索引差值类型
    static constexpr size_type npos           = static_cast<size_type>(-1);                   ///< 查找失败时的返回值
    static constexpr size_type bits           = AllocPolicy::bits;                            ///< 位图总位数
    static constexpr size_type bits_per_block = sizeof(block_type) * CHAR_BIT;                ///< 每块位数
    static constexpr size_type block_count    = (bits + bits_per_block - 1) / bits_per_block; ///< 块数

    static_assert(AllocPolicy::capacity > 0, "BitMap must have a positive number of bits");
    static_assert(detail::has_bits_member_v<AllocPolicy>, "AllocPolicy must have a bits member");
    static_assert(std::is_unsigned_v<typename AllocPolicy::value_type>, "BlockType must be an unsigned integer type");
    static_assert(mu_sstl::is_valid_error_handler_v<ErrorHandler>,
                  "ErrorHandler must have is_fatal, id, and a const call operator() that never returns");
    // ----- 构造 / 析构 -----
    /** @brief 默认构造函数，所有位初始化为 0 */
    BitMap() = default;

    /**
     * @brief 构造一个带有自定义错误处理器的位图
     * @tparam EH 错误处理器类型（必须可转换为 ErrorHandler）
     * @param eh 错误处理器实例（完美转发）
     */
    template <typename EH>
    explicit BitMap(EH&& eh)
        : errorHandler_(std::forward<EH>(eh)) // 注意：这里拷贝构造 errorHandler_，避免多次转发
    {}

    // 禁止拷贝/移动（确保唯一所有权）
    BitMap(const BitMap&)            = delete;
    BitMap& operator=(const BitMap&) = delete;
    BitMap(BitMap&&)                 = delete;
    BitMap& operator=(BitMap&&)      = delete;

    // ----- 按位操作 -----
    /**
     * @brief 将指定位置 1
     * @param pos 位位置（0 ~ bits-1）
     * @note 若 pos 越界，调用错误处理器并传递 ErrorContext{ErrorCode::OutOfBounds, pos}
     */
    inline constexpr void set(size_type pos) {
        if (pos >= bits) errorHandler_(ErrorContext{ErrorCode::OutOfBounds, static_cast<std::uint32_t>(pos)});
        auto& block = blocks_[pos / bits_per_block];
        block |= (static_cast<block_type>(1) << (pos % bits_per_block));
    }

    /**
     * @brief 将指定位置 1
     * @param pos 位位置（0 ~ bits-1）
     * @note 不安全版本，需要用户自己保证 pos 的有效性（通常在性能敏感且已验证索引的场景使用），否则行为未定义
     */
    inline constexpr void unsafe_set(size_type pos) noexcept {
        auto& block = blocks_[pos / bits_per_block];
        block |= (static_cast<block_type>(1) << (pos % bits_per_block));
    }

    /**
     * @brief 将指定位置 0
     * @param pos 位位置（0 ~ bits-1）
     * @note 若 pos 越界，调用错误处理器并传递 ErrorContext{ErrorCode::OutOfBounds, pos}
     */
    inline constexpr void reset(size_type pos) {
        if (pos >= bits) errorHandler_(ErrorContext{ErrorCode::OutOfBounds, static_cast<std::uint32_t>(pos)});
        auto& block = blocks_[pos / bits_per_block];
        block &= ~(static_cast<block_type>(1) << (pos % bits_per_block));
    }

    /**
     * @brief 将指定位置 0
     * @param pos 位位置（0 ~ bits-1）
     * @note 不安全版本，需要用户自己保证 pos 的有效性（通常在性能敏感且已验证索引的场景使用），否则行为未定义
     */
    inline constexpr void unsafe_reset(size_type pos) noexcept {
        auto& block = blocks_[pos / bits_per_block];
        block &= ~(static_cast<block_type>(1) << (pos % bits_per_block));
    }

    /**
     * @brief 翻转指定位置（0 变 1，1 变 0）
     * @param pos 位位置（0 ~ bits-1）
     * @note 若 pos 越界，调用错误处理器并传递 ErrorContext{ErrorCode::OutOfBounds, pos}
     */
    inline constexpr void flip(size_type pos) {
        if (pos >= bits) errorHandler_(ErrorContext{ErrorCode::OutOfBounds, static_cast<std::uint32_t>(pos)});
        auto& block = blocks_[pos / bits_per_block];
        block ^= (static_cast<block_type>(1) << (pos % bits_per_block));
    }

    /**
     * @brief 翻转指定位置（0 变 1，1 变 0）
     * @param pos 位位置（0 ~ bits-1）
     * @note 不安全版本，需要用户自己保证 pos 的有效性（通常在性能敏感且已验证索引的场景使用），否则行为未定义
     */
    inline constexpr void unsafe_flip(size_type pos) noexcept {
        auto& block = blocks_[pos / bits_per_block];
        block ^= (static_cast<block_type>(1) << (pos % bits_per_block));
    }

    /**
     * @brief 测试指定位置是否为 1
     * @param pos 位位置（0 ~ bits-1）
     * @return true 为 1，false 为 0
     * @note 若 pos 越界，调用错误处理器并传递 ErrorContext{ErrorCode::OutOfBounds, pos}
     */
    [[nodiscard]] inline constexpr bool test(size_type pos) const {
        if (pos >= bits) errorHandler_(ErrorContext{ErrorCode::OutOfBounds, static_cast<std::uint32_t>(pos)});
        auto block = blocks_[pos / bits_per_block];
        return (block >> (pos % bits_per_block)) & 1;
    }

    /**
     * @brief 测试指定位置是否为 1
     * @param pos 位位置（0 ~ bits-1）
     * @return true 为 1，false 为 0
     * @note 不安全版本，需要用户自己保证 pos 的有效性（通常在性能敏感且已验证索引的场景使用），否则行为未定义
     */
    [[nodiscard]] inline constexpr bool unsafe_test(size_type pos) const noexcept {
        auto block = blocks_[pos / bits_per_block];
        return (block >> (pos % bits_per_block)) & 1;
    }

    // ----- 范围操作 -----
    /**
     * @brief 将指定范围内的位置1（闭区间 [first, last]）
     * @param first 起始位索引
     * @param last  结束位索引（必须 >= first）
     * @note 若任何索引越界，调用错误处理器并传递 ErrorContext{ErrorCode::OutOfBounds, 越界索引}
     */
    void set_range(size_type first, size_type last) {
        if (first >= bits || last >= bits || first > last) {
            errorHandler_(
                ErrorContext{ErrorCode::OutOfBounds, static_cast<std::uint32_t>(first >= bits ? first : last)});
            return;
        }
        size_type first_block = first / bits_per_block;
        size_type last_block  = last / bits_per_block;
        size_type first_bit   = first % bits_per_block;
        size_type last_bit    = last % bits_per_block;

        if (first_block == last_block) {
            // 同一块内
            block_type mask = ((static_cast<block_type>(1) << (last_bit - first_bit + 1)) - 1) << first_bit;
            blocks_[first_block] |= mask;
        } else {
            // 第一块
            block_type first_mask =
                (first_bit == 0) ? ~block_type{0} : ~((static_cast<block_type>(1) << first_bit) - 1);
            blocks_[first_block] |= first_mask;

            // 中间块全置1
            for (size_type i = first_block + 1; i < last_block; ++i) {
                blocks_[i] = ~block_type{0};
            }

            // 最后一块
            block_type last_mask =
                (last_bit == bits_per_block - 1) ? ~block_type{0} : (static_cast<block_type>(1) << (last_bit + 1)) - 1;
            blocks_[last_block] |= last_mask;
        }
    }

    /**
     * @brief 将指定范围内的位置1（无边界检查）
     * @param first 起始位索引（调用者需保证 first <= last < bits）
     * @param last  结束位索引
     * @note 不安全版本，需要用户自己保证参数有效，否则行为未定义
     */
    void unsafe_set_range(size_type first, size_type last) noexcept {
        size_type first_block = first / bits_per_block;
        size_type last_block  = last / bits_per_block;
        size_type first_bit   = first % bits_per_block;
        size_type last_bit    = last % bits_per_block;

        if (first_block == last_block) {
            block_type mask = ((static_cast<block_type>(1) << (last_bit - first_bit + 1)) - 1) << first_bit;
            blocks_[first_block] |= mask;
        } else {
            block_type first_mask =
                (first_bit == 0) ? ~block_type{0} : ~((static_cast<block_type>(1) << first_bit) - 1);
            blocks_[first_block] |= first_mask;

            for (size_type i = first_block + 1; i < last_block; ++i) {
                blocks_[i] = ~block_type{0};
            }

            block_type last_mask =
                (last_bit == bits_per_block - 1) ? ~block_type{0} : (static_cast<block_type>(1) << (last_bit + 1)) - 1;
            blocks_[last_block] |= last_mask;
        }
    }

    /**
     * @brief 将指定范围内的位置0（闭区间 [first, last]）
     * @param first 起始位索引
     * @param last  结束位索引（必须 >= first）
     * @note 若任何索引越界，调用错误处理器并传递 ErrorContext{ErrorCode::OutOfBounds, 越界索引}
     */
    void reset_range(size_type first, size_type last) {
        if (first >= bits || last >= bits || first > last) {
            errorHandler_(
                ErrorContext{ErrorCode::OutOfBounds, static_cast<std::uint32_t>(first >= bits ? first : last)});
            return;
        }
        size_type first_block = first / bits_per_block;
        size_type last_block  = last / bits_per_block;
        size_type first_bit   = first % bits_per_block;
        size_type last_bit    = last % bits_per_block;

        if (first_block == last_block) {
            block_type mask = ((static_cast<block_type>(1) << (last_bit - first_bit + 1)) - 1) << first_bit;
            blocks_[first_block] &= ~mask;
        } else {
            block_type first_mask =
                (first_bit == 0) ? ~block_type{0} : ~((static_cast<block_type>(1) << first_bit) - 1);
            blocks_[first_block] &= ~first_mask;

            for (size_type i = first_block + 1; i < last_block; ++i) {
                blocks_[i] = 0;
            }

            block_type last_mask =
                (last_bit == bits_per_block - 1) ? ~block_type{0} : (static_cast<block_type>(1) << (last_bit + 1)) - 1;
            blocks_[last_block] &= ~last_mask;
        }
    }

    /**
     * @brief 将指定范围内的位置0（无边界检查）
     * @param first 起始位索引（调用者需保证 first <= last < bits）
     * @param last  结束位索引
     * @note 不安全版本，需要用户自己保证参数有效，否则行为未定义
     */
    void unsafe_reset_range(size_type first, size_type last) noexcept {
        size_type first_block = first / bits_per_block;
        size_type last_block  = last / bits_per_block;
        size_type first_bit   = first % bits_per_block;
        size_type last_bit    = last % bits_per_block;

        if (first_block == last_block) {
            block_type mask = ((static_cast<block_type>(1) << (last_bit - first_bit + 1)) - 1) << first_bit;
            blocks_[first_block] &= ~mask;
        } else {
            block_type first_mask =
                (first_bit == 0) ? ~block_type{0} : ~((static_cast<block_type>(1) << first_bit) - 1);
            blocks_[first_block] &= ~first_mask;

            for (size_type i = first_block + 1; i < last_block; ++i) {
                blocks_[i] = 0;
            }

            block_type last_mask =
                (last_bit == bits_per_block - 1) ? ~block_type{0} : (static_cast<block_type>(1) << (last_bit + 1)) - 1;
            blocks_[last_block] &= ~last_mask;
        }
    }

    // ----- 批量查询 -----
    /** @brief 检查是否有任何位为 1 */
    [[nodiscard]] inline constexpr bool any() const noexcept {
        for (size_type i = 0; i < block_count; ++i) {
            if (blocks_[i] != 0) return true;
        }
        return false;
    }

    /** @brief 检查是否所有位均为 0 */
    [[nodiscard]] inline constexpr bool none() const noexcept {
        return !any();
    }

    /** @brief 返回当前为 1 的位数 */
    [[nodiscard]] size_type count() const noexcept {
        size_type cnt = 0;
        for (size_type i = 0; i < block_count; ++i) {
            cnt += popcount_(blocks_[i]);
        }
        return cnt;
    }

    // ----- 批量设置 -----
    /** @brief 将所有位置 1（最后一块的高位自动清零，以保证有效位正确） */
    inline constexpr void set_all() {
        for (size_type i = 0; i < block_count; ++i) {
            blocks_[i] = ~block_type{0};
        }
        // 清除最后一块的高位（无效位）
        const size_type last_bits = bits % bits_per_block;
        if (last_bits != 0) {
            blocks_[block_count - 1] &= (static_cast<block_type>(1) << last_bits) - 1;
        }
    }

    /** @brief 将所有位置 0 */
    inline constexpr void reset_all() noexcept {
        for (size_type i = 0; i < block_count; ++i) {
            blocks_[i] = 0;
        }
    }

    /** @brief 别名，同 reset_all() */
    inline constexpr void clear() noexcept {
        reset_all();
    }

    // ----- 查找操作 -----
    /**
     * @brief 查找第一个为 1 的位
     * @return 位索引，若无则返回 npos
     */
    [[nodiscard]] size_type find_first_set() const noexcept {
        for (size_type i = 0; i < block_count; ++i) {
            block_type block = blocks_[i];
            if (block != 0) { // 保证块内至少有一位为 1
                size_type bit = find_first_set_in_block_(block);
                return i * bits_per_block + bit;
            }
        }
        return npos;
    }

    /**
     * @brief 查找第一个为 0 的位（忽略无效高位）
     * @return 位索引，若无则返回 npos
     * @note 实现使用掩码技术避免逐位循环，并利用 __builtin_ctz 加速。
     */
    [[nodiscard]] size_type find_first_zero() const noexcept {
        block_type full_mask = ~block_type{0};
        size_type last       = block_count - 1;
        size_type last_bits  = bits % bits_per_block;
        block_type last_mask = (last_bits == 0) ? full_mask : (static_cast<block_type>(1) << last_bits) - 1;

        for (size_type i = 0; i < block_count; ++i) {
            block_type block  = blocks_[i];
            block_type mask   = (i == last) ? last_mask : full_mask;
            block_type masked = block & mask;
            // 以下统一处理，无需再区分最后块
            if (masked != mask) return i * bits_per_block + find_first_zero_in_block_(masked);
        }
        return npos;
    }

    // ----- 容量信息 -----
    /** @brief 返回位图总位数（编译期常量） */
    [[nodiscard]] inline constexpr size_type size() const noexcept {
        return bits;
    }

    /** @brief 检查是否所有位均为 0 */
    [[nodiscard]] inline constexpr bool empty() const noexcept {
        return none();
    }

  private:
    Storage blocks_{};            ///< 底层静态数组存储
    ErrorHandler errorHandler_{}; ///< 错误处理器实例

    // ----- 辅助函数（内置优化或回退实现）-----
    /**
     * @brief 返回整数中 1 的个数
     * @param x 输入值
     * @return 1 的个数
     * @note 优先使用编译器内置函数，否则回退到逐位计数。
     */
    [[nodiscard]] inline static size_type popcount_(block_type x) noexcept {
#if defined(__GNUC__) || defined(__clang__)
        if constexpr (sizeof(block_type) <= sizeof(unsigned int)) {
            return static_cast<size_type>(__builtin_popcount(x));
        } else if constexpr (sizeof(block_type) == sizeof(unsigned long)) {
            return static_cast<size_type>(__builtin_popcountl(x));
        } else if constexpr (sizeof(block_type) == sizeof(unsigned long long)) {
            return static_cast<size_type>(__builtin_popcountll(x));
        }
#else
        // 手动实现：逐位计数
        size_type cnt = 0;
        while (x) {
            cnt += (x & 1);
            x >>= 1;
        }
        return cnt;
#endif
    }

    /**
     * @brief 查找块内第一个为 1 的位
     * @param x 块值（至少有一个 1）
     * @return 位偏移（0 ~ bits_per_block-1）
     * @note 调用者需保证 x 非 0。
     */
    [[nodiscard]] inline static size_type find_first_set_in_block_(block_type x) noexcept {
#if defined(__GNUC__) || defined(__clang__)
        if constexpr (sizeof(block_type) <= sizeof(unsigned int)) {
            return static_cast<size_type>(__builtin_ctz(x));
        } else if constexpr (sizeof(block_type) == sizeof(unsigned long)) {
            return static_cast<size_type>(__builtin_ctzl(x));
        } else if constexpr (sizeof(block_type) == sizeof(unsigned long long)) {
            return static_cast<size_type>(__builtin_ctzll(x));
        }
#else
        // 手动循环
        for (size_type i = 0; i < bits_per_block; ++i) {
            if (x & (static_cast<block_type>(1) << i)) return i;
        }
        return bits_per_block; // 不应到达
#endif
    }

    /**
     * @brief 查找块内第一个为 0 的位
     * @param x 块值（至少有一个 0）
     * @return 位偏移（0 ~ bits_per_block-1）
     * @note 调用者需保证 x 不是全 1。
     */
    [[nodiscard]] inline static size_type find_first_zero_in_block_(block_type x) noexcept {
        return find_first_set_in_block_(~x);
    }
};

/**
 * @brief 默认位图类型别名，使用 std::size_t 作为索引类型
 * @tparam BlockType 块类型（无符号整数）
 * @tparam Bits      位图总位数
 */
template <typename BlockType, std::size_t Bits>
using DefaultBitMap = BitMap<detail::DefaultBitMapAllocPolicy_<BlockType, Bits>>;

} // namespace mu_sstl

#endif // MU_SSTL_BITMAP_HPP
