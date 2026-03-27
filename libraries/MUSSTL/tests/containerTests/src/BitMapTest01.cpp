#include <csetjmp>
#include <cstdint>
#include <type_traits>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "mu_sstl/containers/bit_map.hpp"
#include "mu_sstl/errors/basic_error.hpp"

// 辅助模板：检查类型是否可拷贝/可移动（用于验证拷贝/移动被禁用）
template <typename T>
constexpr bool is_copy_constructible_v = std::is_copy_constructible<T>::value;
template <typename T>
constexpr bool is_move_constructible_v = std::is_move_constructible<T>::value;
template <typename T>
constexpr bool is_copy_assignable_v = std::is_copy_assignable<T>::value;
template <typename T>
constexpr bool is_move_assignable_v = std::is_move_assignable<T>::value;

// 用于错误处理器测试的全局变量和 longjmp 目标
static jmp_buf env;
static bool error_handler_invoked = false;

// 测试用的错误处理器：使用 longjmp 跳出，并设置标志
struct TestErrorHandlerLongjmp {
    using is_fatal               = std::true_type;
    static constexpr uint32_t id = 2001; // 非预留 ID

    [[noreturn]] void operator()(const mu_sstl::ErrorContext&) const {
        std::cout << "TestErrorHandlerLongjmp invoked with code: " << std::hex << std::showbase << id << std::dec << "\n";
        error_handler_invoked = true;
        longjmp(env, 1);
    }
};

namespace mu_sstl
{
// 为测试定义一些常用类型别名
using BM8  = DefaultBitMap<uint32_t, 8>;  // 8位，默认 uint32_t 块
using BM16 = DefaultBitMap<uint16_t, 16>; // 16位，uint16_t 块
using BM32 = DefaultBitMap<uint32_t, 32>; // 32位，uint32_t 块
using BM64 = DefaultBitMap<uint64_t, 64>; // 64位，uint64_t 块
using BM9  = DefaultBitMap<uint32_t, 9>;  // 9位，测试非整数块
using BM17 = DefaultBitMap<uint32_t, 17>; // 17位，测试掩码
using BM65 = DefaultBitMap<uint64_t, 65>; // 65位，测试大位数
} // namespace mu_sstl

// ============================================================================
// 测试用例：构造与容量
// ============================================================================
TEST_CASE("BitMap - construction and capacity") {
    using namespace mu_sstl;

    SUBCASE("Default constructed bitmap is all zero") {
        BM8 bm;
        CHECK(bm.bits == 8);
        CHECK(bm.size() == 8);
        CHECK(bm.empty());
        CHECK_FALSE(bm.any());
        CHECK(bm.none());
        CHECK(bm.count() == 0);
    }

    SUBCASE("Different bit counts and block types") {
        BM16 bm16;
        CHECK(bm16.bits == 16);
        CHECK(bm16.size() == 16);
        CHECK(bm16.empty());

        BM32 bm32;
        CHECK(bm32.bits == 32);
        CHECK(bm32.size() == 32);

        BM64 bm64;
        CHECK(bm64.bits == 64);
        CHECK(bm64.size() == 64);
    }

    SUBCASE("Non-aligned bit counts") {
        BM9 bm9;
        CHECK(bm9.bits == 9);
        CHECK(bm9.size() == 9);

        BM17 bm17;
        CHECK(bm17.bits == 17);
    }
}

// ============================================================================
// 测试用例：按位操作 (set/reset/flip/test)
// ============================================================================
TEST_CASE("BitMap - bit operations") {
    using namespace mu_sstl;

    BM8 bm;

    SUBCASE("Set and test") {
        bm.set(0);
        CHECK(bm.test(0));
        CHECK_FALSE(bm.test(1));

        bm.set(7);
        CHECK(bm.test(7));
        CHECK(bm.test(0));
    }

    SUBCASE("Reset") {
        bm.set(3);
        CHECK(bm.test(3));
        bm.reset(3);
        CHECK_FALSE(bm.test(3));
    }

    SUBCASE("Flip") {
        bm.flip(2);
        CHECK(bm.test(2));
        bm.flip(2);
        CHECK_FALSE(bm.test(2));
    }

    SUBCASE("Multiple operations") {
        bm.set(1);
        bm.set(3);
        bm.set(5);
        CHECK(bm.test(1));
        CHECK(bm.test(3));
        CHECK(bm.test(5));
        CHECK_FALSE(bm.test(0));

        bm.reset(3);
        CHECK_FALSE(bm.test(3));
    }

    SUBCASE("const operations") {
        const BM8 bm;
        CHECK_FALSE(bm.test(0));
        CHECK(bm.none());
    }
}

// ============================================================================
// 测试用例：unsafe 版本（假设索引有效）
// ============================================================================
TEST_CASE("BitMap - unsafe operations") {
    using namespace mu_sstl;

    BM8 bm;

    SUBCASE("unsafe_set and unsafe_test") {
        bm.unsafe_set(0);
        CHECK(bm.unsafe_test(0));
        bm.unsafe_set(7);
        CHECK(bm.unsafe_test(7));
    }

    SUBCASE("unsafe_reset") {
        bm.unsafe_set(4);
        CHECK(bm.unsafe_test(4));
        bm.unsafe_reset(4);
        CHECK_FALSE(bm.unsafe_test(4));
    }

    SUBCASE("unsafe_flip") {
        bm.unsafe_flip(5);
        CHECK(bm.unsafe_test(5));
        bm.unsafe_flip(5);
        CHECK_FALSE(bm.unsafe_test(5));
    }
}

// ============================================================================
// 测试用例：范围操作 (set_range / reset_range)
// ============================================================================
TEST_CASE("BitMap - range operations") {
    using namespace mu_sstl;

    BM17 bm; // 17位，跨块测试

    SUBCASE("Set range within same block") {
        bm.set_range(0, 3); // 位0-3
        for (int i = 0; i <= 3; ++i)
            CHECK(bm.test(i));
        for (int i = 4; i < 17; ++i)
            CHECK_FALSE(bm.test(i));
    }

    SUBCASE("Reset range across blocks") {
        BM65 bm65;
        bm65.set_all(); // 全1
        bm65.reset_range(30, 40);
        for (int i = 0; i < 65; ++i) {
            if (i >= 30 && i <= 40)
                CHECK_FALSE(bm65.test(i));
            else
                CHECK(bm65.test(i));
        }
    }

    SUBCASE("Unsafe range operations") {
        BM65 bm65;
        bm65.unsafe_set_range(0, 31);
        for (int i = 0; i <= 31; ++i)
            CHECK(bm65.test(i));
        for (int i = 32; i < 65; ++i)
            CHECK_FALSE(bm65.test(i));

        bm65.unsafe_reset_range(10, 20);
        for (int i = 0; i <= 9; ++i)
            CHECK(bm65.test(i));
        for (int i = 10; i <= 20; ++i)
            CHECK_FALSE(bm65.test(i));
        for (int i = 21; i <= 31; ++i)
            CHECK(bm65.test(i));
    }

    SUBCASE("Set range across blocks") {
        BM65 bm65;
        bm65.set_range(30, 40);
        for (int i = 0; i < 65; ++i) {
            if (i >= 30 && i <= 40)
                CHECK(bm65.test(i));
            else
                CHECK_FALSE(bm65.test(i));
        }
    }

    SUBCASE("set_range with out-of-bounds last index") {
        using BMAllocPolicy = BitMapAllocPolicy<uint32_t, std::size_t, 8>;
        using BM = BitMap<BMAllocPolicy, TestErrorHandlerLongjmp>;
        BM bm;
        error_handler_invoked = false;
        int val               = setjmp(env);
        if (val == 0) {
            bm.set_range(0, 8);
            CHECK(false);
        } else {
            CHECK(error_handler_invoked);
        }
    }
}

// ============================================================================
// 测试用例：批量查询 (any/none/count)
// ============================================================================
TEST_CASE("BitMap - batch queries") {
    using namespace mu_sstl;

    BM16 bm;

    SUBCASE("Initially none") {
        CHECK(bm.none());
        CHECK_FALSE(bm.any());
        CHECK(bm.count() == 0);
    }

    SUBCASE("After setting some bits") {
        bm.set(3);
        bm.set(7);
        CHECK_FALSE(bm.none());
        CHECK(bm.any());
        CHECK(bm.count() == 2);
    }

    SUBCASE("After resetting all") {
        bm.set(0);
        bm.set(15);
        bm.reset_all();
        CHECK(bm.none());
        CHECK(bm.count() == 0);
    }
}

// ============================================================================
// 测试用例：批量设置 (set_all / reset_all / clear)
// ============================================================================
TEST_CASE("BitMap - batch set/reset") {
    using namespace mu_sstl;

    BM9 bm; // 9位

    SUBCASE("set_all sets all valid bits, clears invalid high bits") {
        bm.set_all();
        for (int i = 0; i < 9; ++i)
            CHECK(bm.test(i));
        // 检查无效高位（如果有）不被设置（无法直接测试，但通过 find_first_zero 可间接验证）
        auto pos = bm.find_first_zero();
        CHECK(pos == BM9::npos); // 应无0位
    }

    SUBCASE("reset_all clears all") {
        bm.set(0);
        bm.set(8);
        bm.reset_all();
        CHECK(bm.none());
    }

    SUBCASE("clear alias") {
        bm.set(4);
        bm.clear();
        CHECK(bm.none());
    }
}

// ============================================================================
// 测试用例：查找操作 (find_first_set / find_first_zero)
// ============================================================================
TEST_CASE("BitMap - find operations") {
    using namespace mu_sstl;

    BM65 bm; // 65位

    SUBCASE("find_first_set on empty returns npos") {
        CHECK(bm.find_first_set() == BM65::npos);
    }

    SUBCASE("find_first_set with bits set") {
        bm.set(10);
        bm.set(20);
        CHECK(bm.find_first_set() == 10);
    }

    SUBCASE("find_first_set with bits in different blocks") {
        bm.set(
            40); // 第二块（64位一块，40在第二块？64位一块，40在第一块，65位用一块？uint64_t一块，65位需要两块，第一块0-63，第二块64-127但只用到64。40在第一块，64在第二块）
        bm.set(64);
        CHECK(bm.find_first_set() == 40);
    }

    SUBCASE("find_first_zero on full returns npos") {
        bm.set_all();
        CHECK(bm.find_first_zero() == BM65::npos);
    }

    SUBCASE("find_first_zero with zeros") {
        bm.set_all();
        bm.reset(50);
        CHECK(bm.find_first_zero() == 50);
    }

    SUBCASE("find_first_zero with multiple zeros") {
        bm.reset(0);
        bm.reset(63);
        bm.set(64);
        // 第一个0是0
        CHECK(bm.find_first_zero() == 0);
    }

    SUBCASE("find_first_zero with last block partially filled") {
        BM17 bm17;
        bm17.set_all();
        bm17.reset(16); // 最后一位
        CHECK(bm17.find_first_zero() == 16);
    }
}

// ============================================================================
// 测试用例：错误处理器（越界操作）
// ============================================================================
TEST_CASE("BitMap - error handler invocation") {
    using namespace mu_sstl;

    SUBCASE("set with out-of-bounds index triggers error handler") {
        using BMAllocPolicy = BitMapAllocPolicy<uint32_t, std::size_t, 8>;
        using BM = BitMap<BMAllocPolicy, TestErrorHandlerLongjmp>;
        BM bm;

        error_handler_invoked = false;
        int val               = setjmp(env);
        if (val == 0) {
            bm.set(8); // 越界
            CHECK(false);
        } else {
            CHECK(error_handler_invoked);
        }
    }

    SUBCASE("reset with out-of-bounds index") {
        using BMAllocPolicy = BitMapAllocPolicy<uint32_t, std::size_t, 8>;
        using BM = BitMap<BMAllocPolicy, TestErrorHandlerLongjmp>;
        BM bm;

        error_handler_invoked = false;
        int val               = setjmp(env);
        if (val == 0) {
            bm.reset(8);
            CHECK(false);
        } else {
            CHECK(error_handler_invoked);
        }
    }

    SUBCASE("flip with out-of-bounds index") {
        using BMAllocPolicy = BitMapAllocPolicy<uint32_t, std::size_t, 8>;
        using BM = BitMap<BMAllocPolicy, TestErrorHandlerLongjmp>;
        BM bm;

        error_handler_invoked = false;
        int val               = setjmp(env);
        if (val == 0) {
            bm.flip(8);
            CHECK(false);
        } else {
            CHECK(error_handler_invoked);
        }
    }

    SUBCASE("test with out-of-bounds index") {
        using BMAllocPolicy = BitMapAllocPolicy<uint32_t, std::size_t, 8>;
        using BM = BitMap<BMAllocPolicy, TestErrorHandlerLongjmp>;
        BM bm;

        error_handler_invoked = false;
        int val               = setjmp(env);
        if (val == 0) {
            (void)bm.test(8);
            CHECK(false);
        } else {
            CHECK(error_handler_invoked);
        }
    }

    SUBCASE("set_range with out-of-bounds first index") {
        using BMAllocPolicy = BitMapAllocPolicy<uint32_t, std::size_t, 8>;
        using BM = BitMap<BMAllocPolicy, TestErrorHandlerLongjmp>;
        BM bm;

        error_handler_invoked = false;
        int val               = setjmp(env);
        if (val == 0) {
            bm.set_range(8, 9);
            CHECK(false);
        } else {
            CHECK(error_handler_invoked);
        }
    }

    SUBCASE("set_range with first > last") {
        using BMAllocPolicy = BitMapAllocPolicy<uint32_t, std::size_t, 8>;
        using BM = BitMap<BMAllocPolicy, TestErrorHandlerLongjmp>;
        BM bm;

        error_handler_invoked = false;
        int val               = setjmp(env);
        if (val == 0) {
            bm.set_range(5, 3);
            CHECK(false);
        } else {
            CHECK(error_handler_invoked);
        }
    }

    SUBCASE("reset_range with out-of-bounds last index") {
        using BMAllocPolicy = BitMapAllocPolicy<uint32_t, std::size_t, 8>;
        using BM = BitMap<BMAllocPolicy, TestErrorHandlerLongjmp>;
        BM bm;

        error_handler_invoked = false;
        int val               = setjmp(env);
        if (val == 0) {
            bm.reset_range(0, 8);
            CHECK(false);
        } else {
            CHECK(error_handler_invoked);
        }
    }
}

// ============================================================================
// 测试用例：自定义错误处理器（非 longjmp 版本）
// ============================================================================
struct CountingHandler {
    static int count;
    using is_fatal               = std::false_type;
    static constexpr uint32_t id = 2002;

    void operator()(const mu_sstl::ErrorContext&) const {
        ++count;
    }
};
int CountingHandler::count = 0;

TEST_CASE("BitMap - custom error handler") {
    using namespace mu_sstl;
        using BMAllocPolicy = BitMapAllocPolicy<uint32_t, std::size_t, 8>;
        using BM = BitMap<BMAllocPolicy, CountingHandler>;
    BM bm;
    CountingHandler::count = 0;

    bm.set(8); // 越界，应触发错误处理器
    CHECK(CountingHandler::count == 1);

    bm.reset(8);
    CHECK(CountingHandler::count == 2);

    bm.flip(8);
    CHECK(CountingHandler::count == 3);
}

// ============================================================================
// 测试用例：拷贝/移动操作被禁用（编译期检查）
// ============================================================================
TEST_CASE("BitMap - copy/move operations disabled") {
    using namespace mu_sstl;
    using BMType = BM8;

    CHECK_FALSE(is_copy_constructible_v<BMType>);
    CHECK_FALSE(is_copy_assignable_v<BMType>);
    CHECK_FALSE(is_move_constructible_v<BMType>);
    CHECK_FALSE(is_move_assignable_v<BMType>);
}

// ============================================================================
// 测试用例：类型别名
// ============================================================================
TEST_CASE("BitMap - type aliases") {
    using namespace mu_sstl;
    using BM = BM8;

    static_assert(std::is_same_v<BM::size_type, std::size_t>);
    static_assert(std::is_same_v<BM::block_type, uint32_t>);
    static_assert(BM::bits == 8);
    static_assert(BM::bits_per_block == 32);
    static_assert(BM::block_count == 1);
}

// ============================================================================
// 测试用例：不同块类型
// ============================================================================
TEST_CASE("BitMap - different block types") {
    using namespace mu_sstl;

    SUBCASE("uint8_t block type") {
        BitMap<detail::DefaultBitMapAllocPolicy_<uint8_t, 16>> bm;
        bm.set(15);
        CHECK(bm.test(15));
        bm.set(7);
        CHECK(bm.test(7));
    }

    SUBCASE("uint16_t block type") {
        BitMap<detail::DefaultBitMapAllocPolicy_<uint16_t, 32>> bm;
        bm.set(31);
        CHECK(bm.test(31));
        bm.set(0);
        CHECK(bm.test(0));
    }

    SUBCASE("uint64_t block type") {
        BitMap<detail::DefaultBitMapAllocPolicy_<uint64_t, 128>> bm;
        bm.set(127);
        CHECK(bm.test(127));
        bm.set(64);
        CHECK(bm.test(64));
    }
}

// ============================================================================
// 测试用例：边界情况（位数为1）
// ============================================================================
TEST_CASE("BitMap - capacity 1") {
    using namespace mu_sstl;
    BitMap<detail::DefaultBitMapAllocPolicy_<uint32_t, 1>> bm;

    CHECK(bm.size() == 1);
    CHECK(bm.none());

    bm.set(0);
    CHECK(bm.test(0));
    CHECK(bm.any());

    bm.reset(0);
    CHECK(bm.none());

    bm.flip(0);
    CHECK(bm.test(0));

    bm.set_all();
    CHECK(bm.test(0));
    CHECK(bm.find_first_zero() == BitMap<detail::DefaultBitMapAllocPolicy_<uint32_t, 1>>::npos);

    bm.reset_all();
    CHECK(bm.none());
}