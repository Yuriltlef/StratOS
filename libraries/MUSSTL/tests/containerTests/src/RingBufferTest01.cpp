#include <csetjmp>
#include <cstdint>
#include <type_traits>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "mu_sstl/containers/ring_buffer.hpp"
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
    static constexpr uint32_t id = 1001; // 非预留 ID

    [[noreturn]] void operator()(const mu_sstl::ErrorContext&) const {
        std::cout << "TestErrorHandlerLongjmp invoked with code: " << std::hex << std::showbase << id << std::dec << "\n";
        error_handler_invoked = true;
        longjmp(env, 1);
    }
};

namespace mu_sstl
{
// 为测试定义一些常用类型别名
using RB8AllocPolicy   = RingBufferAllocPolicy<uint8_t, std::size_t, 4>;
using RB8              = RingBuffer<RB8AllocPolicy>;

using RB16AllocPolicy  = RingBufferAllocPolicy<uint16_t, std::size_t, 8>;
using RB16             = RingBuffer<RB16AllocPolicy>; // 容量 8

using RB32AllocPolicy  = RingBufferAllocPolicy<uint32_t, std::size_t, 16>;
using RB32             = RingBuffer<RB32AllocPolicy>; // 容量 2

using RBIntAllocPolicy = RingBufferAllocPolicy<int, std::size_t, 8>;
using RBInt            = RingBuffer<RBIntAllocPolicy>; // int 类型
struct Point {
    int x, y;
};

using RBPointAllocPolicy = RingBufferAllocPolicy<Point, std::size_t, 3>;
using RBPoint            = RingBuffer<RBPointAllocPolicy>; // 自定义结构体
} // namespace mu_sstl

// ============================================================================
// 测试用例：构造与容量
// ============================================================================
TEST_CASE("RingBuffer - construction and capacity") {
    using namespace mu_sstl;

    SUBCASE("Default constructed buffer is empty") {
        RB8 rb;
        CHECK(rb.capacity == 4);
        CHECK(rb.size() == 0);
        CHECK(rb.max_size() == 4);
        CHECK(rb.empty());
        CHECK_FALSE(rb.full());
    }

    SUBCASE("Different capacity and element type") {
        RB16 rb;
        CHECK(rb.capacity == 8);
        CHECK(rb.size() == 0);
        CHECK(rb.empty());
        CHECK_FALSE(rb.full());
    }
}

// ============================================================================
// 测试用例：available() 容量余量查询
// ============================================================================
TEST_CASE("RingBuffer - available") {
    using namespace mu_sstl;

    DefaultRingBuffer<int, 4> rb; // 容量 4

    // 初始状态：空，可用空间 = 容量
    CHECK(rb.available() == 4);

    // 推入元素后可用空间减少
    rb.push(10);
    CHECK(rb.available() == 3);

    rb.push(20);
    rb.push(30);
    CHECK(rb.available() == 1);

    // 满时可用空间为 0
    rb.push(40);
    CHECK(rb.available() == 0);
    CHECK(rb.full());

    // 弹出后可用空间增加
    rb.pop();
    CHECK(rb.available() == 1);
    rb.pop();
    CHECK(rb.available() == 2);

    // 清空后可用空间恢复为容量
    rb.clear();
    CHECK(rb.available() == 4);
    CHECK(rb.empty());

    // 边界情况：容量为 1
    DefaultRingBuffer<int, 1> rb_small;
    CHECK(rb_small.available() == 1);
    rb_small.push(99);
    CHECK(rb_small.available() == 0);
    rb_small.pop();
    CHECK(rb_small.available() == 1);
}

// ============================================================================
// 测试用例：基本推入和弹出
// ============================================================================
TEST_CASE("RingBuffer - push and pop") {
    using namespace mu_sstl;

    RB8 rb;

    SUBCASE("Push one element") {
        rb.push(10);
        CHECK(rb.size() == 1);
        CHECK_FALSE(rb.empty());
        CHECK_FALSE(rb.full());
        CHECK(rb.front() == 10);
        CHECK(rb.back() == 10);
    }

    SUBCASE("Push multiple elements") {
        rb.push(10);
        rb.push(20);
        rb.push(30);
        CHECK(rb.size() == 3);
        CHECK(rb.front() == 10);
        CHECK(rb.back() == 30);
    }

    SUBCASE("Pop one element") {
        rb.push(10);
        rb.push(20);
        rb.pop();
        CHECK(rb.size() == 1);
        CHECK(rb.front() == 20);
        CHECK(rb.back() == 20);
    }

    SUBCASE("Pop until empty") {
        rb.push(10);
        rb.push(20);
        rb.pop();
        rb.pop();
        CHECK(rb.empty());
    }

    SUBCASE("Circular behavior") {
        // 填满缓冲区
        rb.push(1);
        rb.push(2);
        rb.push(3);
        rb.push(4);
        CHECK(rb.full());

        // 弹出两个，再推入两个，验证索引回绕
        rb.pop(); // 移除 1
        rb.pop(); // 移除 2
        rb.push(5);
        rb.push(6);

        // 此时队列应为 [3,4,5,6]
        CHECK(rb.size() == 4);
        CHECK(rb.front() == 3);
        rb.pop();
        CHECK(rb.front() == 4);
        rb.pop();
        CHECK(rb.front() == 5);
        rb.pop();
        CHECK(rb.front() == 6);
        rb.pop();
        CHECK(rb.empty());
    }
}

// ============================================================================
// 测试用例：满和空状态
// ============================================================================
TEST_CASE("RingBuffer - full and empty") {
    using namespace mu_sstl;

    RB8 rb;

    // 推入直到满
    for (int i = 0; i < 4; ++i) {
        rb.push(static_cast<uint8_t>(i));
    }
    CHECK(rb.full());
    CHECK_FALSE(rb.empty());

    // 弹出直到空
    for (int i = 0; i < 4; ++i) {
        rb.pop();
    }
    CHECK(rb.empty());
    CHECK_FALSE(rb.full());
}

// ============================================================================
// 测试用例：错误处理器（满时 push，空时 pop/front/back）
// ============================================================================
TEST_CASE("RingBuffer - error handler invocation") {
    using namespace mu_sstl;

    SUBCASE("Push when full triggers error handler") {
        using RBAllocPolicy = RingBufferAllocPolicy<uint8_t, std::size_t, 2>;
        using RB            = RingBuffer<RBAllocPolicy, TestErrorHandlerLongjmp>;
        RB rb;

        rb.push(1);
        rb.push(2); // 现在满

        error_handler_invoked = false;
        int val               = setjmp(env);
        if (val == 0) {
            // 第一次进入，尝试推入第三个元素
            rb.push(3);
            // 如果执行到这里，说明错误处理器未调用，测试失败
            CHECK(false);
        } else {
            // longjmp 返回，val == 1
            CHECK(error_handler_invoked);
        }
    }

    SUBCASE("Pop when empty triggers error handler") {
        using RBAllocPolicy = RingBufferAllocPolicy<uint8_t, std::size_t, 2>;
        using RB            = RingBuffer<RBAllocPolicy, TestErrorHandlerLongjmp>;
        RB rb;

        error_handler_invoked = false;
        int val               = setjmp(env);
        if (val == 0) {
            rb.pop();
            CHECK(false);
        } else {
            CHECK(error_handler_invoked);
        }
    }

    SUBCASE("Front when empty triggers error handler") {
        using RBAllocPolicy = RingBufferAllocPolicy<uint8_t, std::size_t, 2>;
        using RB            = RingBuffer<RBAllocPolicy, TestErrorHandlerLongjmp>;
        RB rb;

        error_handler_invoked = false;
        int val               = setjmp(env);
        if (val == 0) {
            (void)rb.front();
            CHECK(false);
        } else {
            CHECK(error_handler_invoked);
        }
    }

    SUBCASE("Back when empty triggers error handler") {
        using RBAllocPolicy = RingBufferAllocPolicy<uint8_t, std::size_t, 2>;
        using RB            = RingBuffer<RBAllocPolicy, TestErrorHandlerLongjmp>;
        RB rb;

        error_handler_invoked = false;
        int val               = setjmp(env);
        if (val == 0) {
            (void)rb.back();
            CHECK(false);
        } else {
            CHECK(error_handler_invoked);
        }
    }
}

// ============================================================================
// 测试用例：try_push / try_pop 安全版本
// ============================================================================
TEST_CASE("RingBuffer - try_push and try_pop") {
    using namespace mu_sstl;

    RB8 rb;

    SUBCASE("try_push succeeds until full") {
        CHECK(rb.try_push(1));
        CHECK(rb.try_push(2));
        CHECK(rb.try_push(3));
        CHECK(rb.try_push(4));
        CHECK_FALSE(rb.try_push(5)); // 已满，返回 false
        CHECK(rb.full());
    }

    SUBCASE("try_pop succeeds until empty") {
        rb.push(10);
        rb.push(20);

        uint8_t out;
        CHECK(rb.try_pop(out));
        CHECK(out == 10);
        CHECK(rb.try_pop(out));
        CHECK(out == 20);
        CHECK_FALSE(rb.try_pop(out)); // 空，返回 false
    }

    SUBCASE("try_pop does not modify buffer on failure") {
        uint8_t out = 0;
        CHECK_FALSE(rb.try_pop(out));
        CHECK(out == 0); // 未被修改
    }
}

// ============================================================================
// 测试用例：clear 方法
// ============================================================================
TEST_CASE("RingBuffer - clear") {
    using namespace mu_sstl;

    RB8 rb;
    rb.push(1);
    rb.push(2);
    rb.push(3);
    rb.clear();

    CHECK(rb.empty());
    CHECK_FALSE(rb.full());
    CHECK(rb.size() == 0);

    // 清空后可以重新使用
    rb.push(5);
    CHECK(rb.front() == 5);
}

// ============================================================================
// 测试用例：swap 方法
// ============================================================================
TEST_CASE("RingBuffer - swap") {
    using namespace mu_sstl;

    RB8 rb1, rb2;
    rb1.push(1);
    rb1.push(2);
    rb2.push(10);
    rb2.push(20);

    rb1.swap(rb2);

    CHECK(rb1.front() == 10);
    CHECK(rb1.back() == 20);
    CHECK(rb2.front() == 1);
    CHECK(rb2.back() == 2);

    // 非成员 swap
    swap(rb1, rb2);
    CHECK(rb1.front() == 1);
    CHECK(rb2.front() == 10);
}

// ============================================================================
// 测试用例：拷贝/移动操作被禁用（编译期检查）
// ============================================================================
TEST_CASE("RingBuffer - copy/move operations disabled") {
    using namespace mu_sstl;
    using RBType = RB8;

    CHECK_FALSE(is_copy_constructible_v<RBType>);
    CHECK_FALSE(is_copy_assignable_v<RBType>);
    CHECK_FALSE(is_move_constructible_v<RBType>);
    CHECK_FALSE(is_move_assignable_v<RBType>);
}

// ============================================================================
// 测试用例：类型别名
// ============================================================================
TEST_CASE("RingBuffer - type aliases") {
    using namespace mu_sstl;
    using RB = RB8;

    static_assert(std::is_same_v<RB::value_type, uint8_t>);
    static_assert(std::is_same_v<RB::size_type, std::size_t>);
    static_assert(std::is_same_v<RB::reference, uint8_t&>);
    static_assert(std::is_same_v<RB::const_reference, const uint8_t&>);
    static_assert(std::is_same_v<RB::pointer, uint8_t*>);
    static_assert(std::is_same_v<RB::const_pointer, const uint8_t*>);
    // 注意：difference_type 从 Storage 继承，通常为 std::ptrdiff_t
    // 但这里我们不深入检查，因为 Storage 是 StaticArray 的，已测试
}

// ============================================================================
// 测试用例：不同元素类型
// ============================================================================
TEST_CASE("RingBuffer - different element types") {
    using namespace mu_sstl;

    SUBCASE("uint16_t buffer") {
        RB16 rb;
        rb.push(1000);
        rb.push(2000);
        CHECK(rb.front() == 1000);
        CHECK(rb.back() == 2000);
    }

    SUBCASE("int buffer") {
        RBInt rb;
        rb.push(-10);
        rb.push(20);
        CHECK(rb.front() == -10);
        CHECK(rb.back() == 20);
    }

    SUBCASE("custom struct buffer") {
        RBPoint rb;
        rb.push(Point{1, 2});
        rb.push(Point{3, 4});
        Point p = rb.front();
        CHECK(p.x == 1);
        CHECK(p.y == 2);
        rb.pop();
        p = rb.front();
        CHECK(p.x == 3);
        CHECK(p.y == 4);
    }
}

struct CountingHandler {
    static int count;
    using is_fatal               = std::false_type;
    static constexpr uint32_t id = 1002; // 非预留 ID

    void operator()(const mu_sstl::ErrorContext&) const {
        std::cout << "CountingHandler invoked for error code: " << std::hex << std::showbase << id << std::dec << "\n";
        ++count;
    }
};
int CountingHandler::count = 0;

// ============================================================================
// 测试用例：自定义错误处理器（非 longjmp 版本）
// ============================================================================
TEST_CASE("RingBuffer - custom error handler") {
    using namespace mu_sstl;
    using RBAllocPolicy = RingBufferAllocPolicy<uint8_t, std::size_t, 2>;
    using RB            = RingBuffer<RBAllocPolicy, CountingHandler>;
    RB rb;
    CountingHandler::count = 0; // 重置计数

    rb.push(1);
    rb.push(2);
    rb.push(3); // 应触发错误处理器

    CHECK(CountingHandler::count == 1);
}

// ============================================================================
// 测试用例：边界情况（容量为1）
// ============================================================================
TEST_CASE("RingBuffer - capacity 1") {
    using namespace mu_sstl;
    DefaultRingBuffer<int, 1> rb;

    CHECK(rb.empty());
    CHECK_FALSE(rb.full());

    rb.push(42);
    CHECK(rb.full());
    CHECK(rb.front() == 42);
    CHECK(rb.back() == 42);

    int out;
    CHECK(rb.try_pop(out));
    CHECK(out == 42);
    CHECK(rb.empty());

    // 满时尝试 push
    CHECK(rb.try_push(100));
    CHECK(rb.full());
    CHECK_FALSE(rb.try_push(200));
}
