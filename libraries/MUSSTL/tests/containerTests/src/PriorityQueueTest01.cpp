#include <csetjmp>
#include <cstdint>
#include <iostream>
#include <type_traits>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS

#include "doctest/doctest.h"
#include "mu_sstl/containers/priority_queue.hpp"
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
    static constexpr uint32_t id = 3001; // 非预留 ID

    [[noreturn]] void operator()(const mu_sstl::ErrorContext&) const {
        std::cout << "TestErrorHandlerLongjmp invoked with code: " << std::hex << std::showbase << id << std::dec << "\n";
        error_handler_invoked = true;
        longjmp(env, 1);
    }
};

namespace mu_sstl
{
// 为测试定义一些常用类型别名
// 最大堆，int，容量 8
using PQIntMaxAllocPolicy = PriorityQueueAllocPolicy<int, std::size_t, 8, QueueType::MaxHeap>;
using PQIntMax = PriorityQueue<PQIntMaxAllocPolicy>;

// 最小堆，int，容量 8
using PQIntMinAllocPolicy = PriorityQueueAllocPolicy<int, std::size_t, 8, QueueType::MinHeap>;
using PQIntMin = PriorityQueue<PQIntMinAllocPolicy>;

// 自定义结构体
struct Point {
    int x, y;
    bool operator<(const Point& other) const { return x < other.x; } // 用于最大堆
    bool operator>(const Point& other) const { return x > other.x; } // 用于最小堆
};

using PQPointMaxAllocPolicy = PriorityQueueAllocPolicy<Point, std::size_t, 5, QueueType::MaxHeap>;
using PQPointMax = PriorityQueue<PQPointMaxAllocPolicy>;

using PQPointMinAllocPolicy = PriorityQueueAllocPolicy<Point, std::size_t, 5, QueueType::MinHeap>;
using PQPointMin = PriorityQueue<PQPointMinAllocPolicy>;

// 容量为1的特殊情况
using PQInt1AllocPolicy = PriorityQueueAllocPolicy<int, std::size_t, 1, QueueType::MaxHeap>;
using PQInt1 = PriorityQueue<PQInt1AllocPolicy>;

} // namespace mu_sstl

// ============================================================================
// 测试用例：构造与容量
// ============================================================================
TEST_CASE("PriorityQueue - construction and capacity") {
    using namespace mu_sstl;

    SUBCASE("Default constructed queue is empty") {
        PQIntMax pq;
        CHECK(pq.capacity == 8);
        CHECK(pq.size() == 0);
        CHECK(pq.max_size() == 8);
        CHECK(pq.empty());
        CHECK_FALSE(pq.full());
        CHECK(pq.available() == 8);
    }

    SUBCASE("Different capacity and element type") {
        PQIntMin pq;
        CHECK(pq.capacity == 8);
        CHECK(pq.size() == 0);
    }
}

// ============================================================================
// 测试用例：available() 容量余量查询
// ============================================================================
TEST_CASE("PriorityQueue - available") {
    using namespace mu_sstl;

    PQIntMax pq; // 容量 8

    // 初始状态：空，可用空间 = 容量
    CHECK(pq.available() == 8);

    // 插入元素后可用空间减少
    pq.push(10);
    CHECK(pq.available() == 7);

    pq.push(20);
    pq.push(30);
    CHECK(pq.available() == 5);

    // 满时可用空间为 0
    for (int i = 0; i < 5; ++i) pq.push(i); // 共 8 个
    CHECK(pq.available() == 0);
    CHECK(pq.full());

    // 删除后可用空间增加
    pq.pop();
    CHECK(pq.available() == 1);
    pq.pop();
    CHECK(pq.available() == 2);

    // 清空后可用空间恢复为容量
    pq.clear();
    CHECK(pq.available() == 8);
    CHECK(pq.empty());

    // 边界情况：容量为 1
    PQInt1 pq_small;
    CHECK(pq_small.available() == 1);
    pq_small.push(99);
    CHECK(pq_small.available() == 0);
    pq_small.pop();
    CHECK(pq_small.available() == 1);
}

// ============================================================================
// 测试用例：最大堆基本操作
// ============================================================================
TEST_CASE("PriorityQueue - max heap operations") {
    using namespace mu_sstl;

    PQIntMax pq;

    SUBCASE("Push and top") {
        pq.push(10);
        CHECK(pq.top() == 10);
        pq.push(20);
        CHECK(pq.top() == 20);
        pq.push(5);
        CHECK(pq.top() == 20);
        pq.push(15);
        CHECK(pq.top() == 20);
    }

    SUBCASE("Pop and maintain heap property") {
        pq.push(10);
        pq.push(30);
        pq.push(20);
        pq.push(5);
        pq.push(25);

        CHECK(pq.top() == 30);
        pq.pop();
        CHECK(pq.top() == 25);
        pq.pop();
        CHECK(pq.top() == 20);
        pq.pop();
        CHECK(pq.top() == 10);
        pq.pop();
        CHECK(pq.top() == 5);
        pq.pop();
        CHECK(pq.empty());
    }

    SUBCASE("Pop until empty") {
        pq.push(1);
        pq.push(2);
        pq.push(3);
        pq.pop();
        pq.pop();
        pq.pop();
        CHECK(pq.empty());
    }
}

// ============================================================================
// 测试用例：最小堆基本操作
// ============================================================================
TEST_CASE("PriorityQueue - min heap operations") {
    using namespace mu_sstl;

    PQIntMin pq;

    SUBCASE("Push and top") {
        pq.push(10);
        CHECK(pq.top() == 10);
        pq.push(20);
        CHECK(pq.top() == 10);
        pq.push(5);
        CHECK(pq.top() == 5);
        pq.push(15);
        CHECK(pq.top() == 5);
    }

    SUBCASE("Pop and maintain heap property") {
        pq.push(10);
        pq.push(30);
        pq.push(20);
        pq.push(5);
        pq.push(25);

        CHECK(pq.top() == 5);
        pq.pop();
        CHECK(pq.top() == 10);
        pq.pop();
        CHECK(pq.top() == 20);
        pq.pop();
        CHECK(pq.top() == 25);
        pq.pop();
        CHECK(pq.top() == 30);
        pq.pop();
        CHECK(pq.empty());
    }
}

// ============================================================================
// 测试用例：满和空状态
// ============================================================================
TEST_CASE("PriorityQueue - full and empty") {
    using namespace mu_sstl;

    PQIntMax pq; // 容量 8

    // 插入直到满
    for (int i = 0; i < 8; ++i) {
        pq.push(i);
    }
    CHECK(pq.full());
    CHECK_FALSE(pq.empty());
    CHECK(pq.available() == 0);

    // 删除直到空
    for (int i = 0; i < 8; ++i) {
        pq.pop();
    }
    CHECK(pq.empty());
    CHECK_FALSE(pq.full());
    CHECK(pq.available() == 8);
}

// ============================================================================
// 测试用例：错误处理器（满时 push，空时 pop/top）
// ============================================================================
TEST_CASE("PriorityQueue - error handler invocation") {
    using namespace mu_sstl;

    // 使用容量为 2 的队列，测试满和空
    using PQAllocPolicy = PriorityQueueAllocPolicy<int, std::size_t, 2, QueueType::MaxHeap>;
    using PQWithError = PriorityQueue<PQAllocPolicy, TestErrorHandlerLongjmp>;

    SUBCASE("Push when full triggers error handler") {
        PQWithError pq;
        pq.push(1);
        pq.push(2); // 现在满

        error_handler_invoked = false;
        int val = setjmp(env);
        if (val == 0) {
            pq.push(3);
            CHECK(false);
        } else {
            CHECK(error_handler_invoked);
        }
    }

    SUBCASE("Pop when empty triggers error handler") {
        PQWithError pq;

        error_handler_invoked = false;
        int val = setjmp(env);
        if (val == 0) {
            pq.pop();
            CHECK(false);
        } else {
            CHECK(error_handler_invoked);
        }
    }

    SUBCASE("Top when empty triggers error handler") {
        PQWithError pq;

        error_handler_invoked = false;
        int val = setjmp(env);
        if (val == 0) {
            (void)pq.top();
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
    static constexpr uint32_t id = 3002;

    void operator()(const mu_sstl::ErrorContext&) const {
        std::cout << "CountingHandler invoked\n";
        ++count;
    }
};
int CountingHandler::count = 0;

TEST_CASE("PriorityQueue - custom error handler") {
    using namespace mu_sstl;
    using PQAllocPolicy = PriorityQueueAllocPolicy<int, std::size_t, 2, QueueType::MaxHeap>;
    using PQWithHandler = PriorityQueue<PQAllocPolicy, CountingHandler>;
    PQWithHandler pq;
    CountingHandler::count = 0;

    pq.push(1);
    pq.push(2);
    pq.push(3); // 满，触发错误处理器

    CHECK(CountingHandler::count == 1);
}

// ============================================================================
// 测试用例：clear 方法
// ============================================================================
TEST_CASE("PriorityQueue - clear") {
    using namespace mu_sstl;

    PQIntMax pq;
    pq.push(1);
    pq.push(2);
    pq.push(3);
    pq.clear();

    CHECK(pq.empty());
    CHECK(pq.size() == 0);
    CHECK(pq.available() == pq.capacity);

    // 清空后可以重新使用
    pq.push(5);
    CHECK(pq.top() == 5);
}

// ============================================================================
// 测试用例：swap 方法
// ============================================================================
TEST_CASE("PriorityQueue - swap") {
    using namespace mu_sstl;

    PQIntMax pq1, pq2;
    pq1.push(1);
    pq1.push(2);
    pq1.push(3);
    pq2.push(10);
    pq2.push(20);

    pq1.swap(pq2);

    CHECK(pq1.size() == 2);
    CHECK(pq1.top() == 20); // 最大堆
    pq1.pop();
    CHECK(pq1.top() == 10);

    CHECK(pq2.size() == 3);
    CHECK(pq2.top() == 3);
    pq2.pop();
    CHECK(pq2.top() == 2);

    // 非成员 swap
    swap(pq1, pq2);
    CHECK(pq1.size() == 2);
    CHECK(pq2.size() == 1);
}

// ============================================================================
// 测试用例：拷贝/移动操作被禁用（编译期检查）
// ============================================================================
TEST_CASE("PriorityQueue - copy/move operations disabled") {
    using namespace mu_sstl;
    using PQType = PQIntMax;

    CHECK_FALSE(is_copy_constructible_v<PQType>);
    CHECK_FALSE(is_copy_assignable_v<PQType>);
    CHECK_FALSE(is_move_constructible_v<PQType>);
    CHECK_FALSE(is_move_assignable_v<PQType>);
}

// ============================================================================
// 测试用例：类型别名
// ============================================================================
TEST_CASE("PriorityQueue - type aliases") {
    using namespace mu_sstl;
    using PQ = PQIntMax;

    static_assert(std::is_same_v<PQ::value_type, int>);
    static_assert(std::is_same_v<PQ::size_type, std::size_t>);
    static_assert(std::is_same_v<PQ::reference, int&>);
    static_assert(std::is_same_v<PQ::const_reference, const int&>);
    static_assert(std::is_same_v<PQ::pointer, int*>);
    static_assert(std::is_same_v<PQ::const_pointer, const int*>);
    // difference_type 从 Storage 继承，通常为 std::ptrdiff_t，但此处不深入检查
}

// ============================================================================
// 测试用例：不同元素类型和比较器
// ============================================================================
TEST_CASE("PriorityQueue - different element types") {
    using namespace mu_sstl;

    SUBCASE("int, max heap") {
        PQIntMax pq;
        pq.push(100);
        pq.push(200);
        CHECK(pq.top() == 200);
    }

    SUBCASE("int, min heap") {
        PQIntMin pq;
        pq.push(100);
        pq.push(200);
        CHECK(pq.top() == 100);
    }

    SUBCASE("custom struct, max heap") {
        PQPointMax pq;
        pq.push(Point{1,2});
        pq.push(Point{3,4});
        pq.push(Point{2,5});
        CHECK(pq.top().x == 3);
    }

    SUBCASE("custom struct, min heap") {
        PQPointMin pq;
        pq.push(Point{1,2});
        pq.push(Point{3,4});
        pq.push(Point{2,5});
        CHECK(pq.top().x == 1);
    }
}

// ============================================================================
// 测试用例：边界情况（容量为1）
// ============================================================================
TEST_CASE("PriorityQueue - capacity 1") {
    using namespace mu_sstl;

    PQInt1 pq;
    CHECK(pq.empty());
    CHECK_FALSE(pq.full());

    pq.push(42);
    CHECK(pq.full());
    CHECK(pq.top() == 42);

    pq.pop();
    CHECK(pq.empty());

    // 满时尝试 push（会触发错误处理器，用 longjmp 测试）
    using PQAllocPolicy = PriorityQueueAllocPolicy<int, std::size_t, 1, QueueType::MaxHeap>;
    using PQWithError = PriorityQueue<PQAllocPolicy, TestErrorHandlerLongjmp>;
    PQWithError pq_err;
    pq_err.push(1);
    error_handler_invoked = false;
    int val = setjmp(env);
    if (val == 0) {
        pq_err.push(2);
        CHECK(false);
    } else {
        CHECK(error_handler_invoked);
    }
}