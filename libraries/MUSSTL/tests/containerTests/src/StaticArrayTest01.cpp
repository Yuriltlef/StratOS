#include <cstdint>
#include <type_traits>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "mu_sstl/containers/static_array.hpp"
#include "mu_sstl/errors/basic_error.hpp"

// 用于验证永不返回的另一个处理器（无抛出，但测试中不会实际调用）
struct NoReturnHandler {
    using is_fatal               = std::true_type;
    static constexpr uint32_t id = 999; // 非预留 ID

    [[noreturn]] void operator()(const mu_sstl::ErrorContext&) const {
        while (true) {}
    }
};

// 辅助模板：检查类型是否可拷贝/可移动
template <typename T>
constexpr bool is_copy_constructible_v = std::is_copy_constructible<T>::value;
template <typename T>
constexpr bool is_move_constructible_v = std::is_move_constructible<T>::value;
template <typename T>
constexpr bool is_copy_assignable_v = std::is_copy_assignable<T>::value;
template <typename T>
constexpr bool is_move_assignable_v = std::is_move_assignable<T>::value;

namespace mu_sstl
{
// 为测试定义一些策略别名
using TestPolicy1 = StaticAllocPolicy<uint32_t, std::size_t, 10>;        // 默认对齐 = alignof(uint32_t)
using TestPolicy2 = StaticAllocPolicy<uint8_t, uint8_t, 5, 2>;           // 小容量，小 size_type
using TestPolicy3 = StaticAllocPolicy<int, uint16_t, 100, alignof(int)>; // 带符号元素类型
using TestPolicy4 = StaticAllocPolicy<double, uint32_t, 3, 8>;           // double 类型
} // namespace mu_sstl

// ============================================================================
// 测试用例：构造与容量信息
// ============================================================================
TEST_CASE("StaticArray - construction and capacity") {
    using namespace mu_sstl;
    StaticArray<TestPolicy1> arr1;
    SUBCASE("Default constructed array is zero-initialized") {
        StaticArray<TestPolicy1> arr;
        CHECK(arr.capacity == 10);
        CHECK(arr.size() == 10);
        CHECK(arr.max_size() == 10);
        CHECK_FALSE(arr.empty()); // 容量 > 0
        CHECK(arr.size_bytes() == sizeof(uint32_t) * 10);
        // 验证零初始化（此处只检查第一个元素，完整测试见专门用例）
        CHECK(arr[0] == 0);
    }

    SUBCASE("Array with small size_type (uint8_t)") {
        StaticArray<TestPolicy2> arr;
        CHECK(arr.capacity == 5);
        CHECK(arr.size() == 5);
        CHECK(arr.max_size() == 5);
        CHECK(arr.size_bytes() == sizeof(uint8_t) * 5);
    }

    SUBCASE("Array with signed element type") {
        StaticArray<TestPolicy3> arr;
        arr.fill(-1); // 填充负数测试
        CHECK(arr[0] == -1);
    }

    SUBCASE("Array with alignment specification") {
        StaticArray<TestPolicy2> arr; // 对齐要求 2
        CHECK(arr.alignment == 2);
        // 对齐的验证依赖编译器，我们只检查静态值
        StaticArray<TestPolicy4> arr2; // 对齐要求 8
        CHECK(arr2.alignment == 8);
    }
}

// ============================================================================
// 测试用例：元素访问 (at / operator[] / front / back)
// ============================================================================
TEST_CASE("StaticArray - element access") {
    using namespace mu_sstl;

    StaticArray<TestPolicy1> arr;
    // 填充一些值
    for (std::size_t i = 0; i < arr.capacity; ++i) {
        arr[i] = static_cast<uint32_t>(i * 2);
    }

    SUBCASE("at() returns correct reference") {
        CHECK(arr.at(0) == 0);
        CHECK(arr.at(5) == 10);
        arr.at(3) = 100;
        CHECK(arr.at(3) == 100);
    }

    SUBCASE("const at() works on const object") {
        const auto& carr = arr;
        CHECK(carr.at(1) == 2);
        // carr.at(1) = 10;  // 编译错误，符合预期
    }

    SUBCASE("operator[] provides unchecked access") {
        CHECK(arr[2] == 4);
        arr[2] = 99;
        CHECK(arr[2] == 99);
    }

    SUBCASE("front() and back()") {
        CHECK(arr.front() == 0);
        CHECK(arr.back() == 18); // 最后一个索引 9, 值 18
        arr.front() = 123;
        arr.back()  = 456;
        CHECK(arr[0] == 123);
        CHECK(arr[9] == 456);

        const auto& carr = arr;
        CHECK(carr.front() == 123);
        CHECK(carr.back() == 456);
    }
}

#include <csetjmp>

static jmp_buf env;
static bool error_handler_invoked = false;

struct TestErrorHandlerLongjmp {
    using is_fatal               = std::true_type;
    static constexpr uint32_t id = 1000; // 非预留 ID

    [[noreturn]] void operator()(const mu_sstl::ErrorContext&) const {
        error_handler_invoked = true;
        longjmp(env, 1);
    }
};

TEST_CASE("StaticArray - bounds checking with longjmp") {
    using namespace mu_sstl;
    using Array = StaticArray<TestPolicy1, TestErrorHandlerLongjmp>;
    Array arr;

    error_handler_invoked = false;
    int val               = setjmp(env);
    if (val == 0) {
        // 第一次进入，尝试越界访问
        auto _x = arr.at(10);
        // 如果执行到这里，说明越界未触发，测试失败
        CHECK(false);
    } else {
        // longjmp 返回，val == 1
        CHECK(error_handler_invoked);
    }
}

// ============================================================================
// 测试用例：fill 方法
// ============================================================================
TEST_CASE("StaticArray - fill") {
    using namespace mu_sstl;

    StaticArray<TestPolicy2> arr; // uint8_t[5]
    arr.fill(0xAA);
    for (uint8_t i = 0; i < arr.capacity; ++i) {
        CHECK(arr[i] == 0xAA);
    }

    arr.fill(0x55);
    for (uint8_t i = 0; i < arr.capacity; ++i) {
        CHECK(arr[i] == 0x55);
    }
}

// ============================================================================
// 测试用例：swap 方法
// ============================================================================
TEST_CASE("StaticArray - swap") {
    using namespace mu_sstl;

    StaticArray<TestPolicy2> arr1, arr2;
    arr1.fill(0x11);
    arr2.fill(0x22);

    arr1.swap(arr2);
    for (uint8_t i = 0; i < arr1.capacity; ++i) {
        CHECK(arr1[i] == 0x22);
        CHECK(arr2[i] == 0x11);
    }

    // 非成员 swap
    swap(arr1, arr2);
    for (uint8_t i = 0; i < arr1.capacity; ++i) {
        CHECK(arr1[i] == 0x11);
        CHECK(arr2[i] == 0x22);
    }
}

// ============================================================================
// 测试用例：迭代器
// ============================================================================
TEST_CASE("StaticArray - iterators") {
    using namespace mu_sstl;

    StaticArray<TestPolicy1> arr; // uint32_t[10]
    for (std::size_t i = 0; i < arr.capacity; ++i) {
        arr[i] = static_cast<uint32_t>(i);
    }

    SUBCASE("range-based for loop") {
        std::size_t idx = 0;
        for (auto& v : arr) {
            CHECK(v == idx);
            ++idx;
        }
        CHECK(idx == arr.capacity);
    }

    SUBCASE("const range-based for loop") {
        const auto& carr = arr;
        std::size_t idx  = 0;
        for (const auto& v : carr) {
            CHECK(v == idx);
            ++idx;
        }
    }

    SUBCASE("iterator comparison and increment") {
        auto it  = arr.begin();
        auto end = arr.end();
        CHECK(it != end);
        CHECK(it == arr.begin());
        ++it;
        CHECK(*it == 1);
        auto it2 = it++;
        CHECK(*it2 == 1); // 后置++返回原值
        CHECK(*it == 2);
    }

    SUBCASE("const_iterator") {
        const auto& carr = arr;
        auto cit         = carr.cbegin();
        CHECK(*cit == 0);
        ++cit;
        CHECK(*cit == 1);
        // CHECK(cit == carr.begin() + 1);  // 此处 +1 不可用，但可比较
        // 验证 const 迭代器不能修改元素
        // *cit = 10;   // 编译错误
    }
}

// ============================================================================
// 测试用例：零初始化保证
// ============================================================================
TEST_CASE("StaticArray - zero initialization") {
    using namespace mu_sstl;

    StaticArray<TestPolicy3> arr; // int[100]
    for (uint16_t i = 0; i < arr.capacity; ++i) {
        CHECK(arr[i] == 0); // int 应被零初始化
    }

    // 对于 POD 类型，默认构造后全零
    StaticArray<TestPolicy2> arr2; // uint8_t[5]
    for (uint8_t i = 0; i < arr2.capacity; ++i) {
        CHECK(arr2[i] == 0);
    }
}

// ============================================================================
// 测试用例：对齐属性
// ============================================================================
TEST_CASE("StaticArray - alignment") {
    using namespace mu_sstl;

    // 验证 alignment 静态成员与策略一致
    CHECK(StaticArray<TestPolicy1>::alignment == alignof(uint32_t));
    CHECK(StaticArray<TestPolicy2>::alignment == 2);
    CHECK(StaticArray<TestPolicy4>::alignment == 8);

    // 验证实际对象地址是否符合对齐要求（编译期无法保证，运行时检查）
    StaticArray<TestPolicy2> arr2;
    CHECK((reinterpret_cast<std::uintptr_t>(arr2.data()) % 2) == 0);

    StaticArray<TestPolicy4> arr4;
    CHECK((reinterpret_cast<std::uintptr_t>(arr4.data()) % 8) == 0);
}

// ============================================================================
// 测试用例：不同策略组合
// ============================================================================
TEST_CASE("StaticArray - different policy combinations") {
    using namespace mu_sstl;

    SUBCASE("uint8_t, size_type = uint8_t, capacity = 3") {
        using Policy = StaticAllocPolicy<uint8_t, uint8_t, 3>;
        StaticArray<Policy> arr;
        CHECK(arr.capacity == 3);
        arr[0] = 1;
        arr[1] = 2;
        arr[2] = 3;
        CHECK(arr[0] == 1);
        CHECK(arr[1] == 2);
        CHECK(arr[2] == 3);
    }

    SUBCASE("int, size_type = uint16_t, capacity = 1000") {
        using Policy = StaticAllocPolicy<int, uint16_t, 1000>;
        StaticArray<Policy> arr;
        arr.fill(42);
        CHECK(arr[999] == 42);
        CHECK(arr.size_bytes() == sizeof(int) * 1000);
    }

    SUBCASE("Custom struct type") {
        struct Point {
            int x, y;
        };
        using Policy = StaticAllocPolicy<Point, std::size_t, 2>;
        StaticArray<Policy> arr;
        arr[0] = Point{1, 2};
        arr[1] = Point{3, 4};
        CHECK(arr[0].x == 1);
        CHECK(arr[1].y == 4);
    }
}

// ============================================================================
// 测试用例：拷贝/移动操作被禁用（编译期检查）
// ============================================================================
TEST_CASE("StaticArray - copy/move operations disabled") {
    using namespace mu_sstl;
    using ArrayType = StaticArray<TestPolicy1>;

    // 这些检查在编译期完成，但我们可以用静态断言或 CHECK 表达式
    CHECK_FALSE(is_copy_constructible_v<ArrayType>);
    CHECK_FALSE(is_copy_assignable_v<ArrayType>);
    CHECK_FALSE(is_move_constructible_v<ArrayType>);
    CHECK_FALSE(is_move_assignable_v<ArrayType>);
}

// ============================================================================
// 测试用例：data() 方法
// ============================================================================
TEST_CASE("StaticArray - data()") {
    using namespace mu_sstl;
    StaticArray<TestPolicy1> arr;
    arr[0]        = 0xDEADBEEF;
    uint32_t* ptr = arr.data();
    CHECK(ptr[0] == 0xDEADBEEF);
    ptr[1] = 0xCAFEBABE;
    CHECK(arr[1] == 0xCAFEBABE);

    const auto& carr     = arr;
    const uint32_t* cptr = carr.data();
    CHECK(cptr[0] == 0xDEADBEEF);
    CHECK(cptr[1] == 0xCAFEBABE);
}