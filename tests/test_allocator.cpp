// tests/test_allocator.cpp
// Tests for kalloc<T>(), kalloc_array<T>(), kfree_obj(), and GFP flag selection.
#include "cpp_lkm/runtime/kalloc.hpp"
#include "tests/support/mock_globals.hpp"

#include <assert.h>
#include <stdio.h>

struct TestObj
{
    int x;
    static int destructor_count;

    explicit TestObj(int val) : x(val) {}
    ~TestObj()
    {
        ++destructor_count;
    }
};

int TestObj::destructor_count = 0;

// ---- GFP flag selection ----

void test_kalloc_success()
{
    reset_mock_state();
    auto p = kalloc<TestObj>(42);
    assert(p.has_value());
    assert(p.value()->x == 42);
    assert(__mock_last_gfp == CPP_GFP_KERNEL);
    kfree_obj(*p);
}

void test_kalloc_atomic()
{
    reset_mock_state();
    __mock_preempt_count = 1; // in_atomic() → true
    auto p = kalloc<TestObj>(10);
    assert(p.has_value());
    assert(__mock_last_gfp == CPP_GFP_ATOMIC);
    kfree_obj(*p);
}

void test_kalloc_irqs_disabled()
{
    reset_mock_state();
    __mock_irqs_disabled = 1;
    auto p = kalloc<TestObj>(10);
    assert(p.has_value());
    assert(__mock_last_gfp == CPP_GFP_ATOMIC);
    kfree_obj(*p);
}

void test_kalloc_nmi()
{
    reset_mock_state();
    __mock_in_nmi = 1;
    auto p = kalloc<TestObj>(10);
    assert(p.has_value());
    assert(__mock_last_gfp == CPP_GFP_ATOMIC);
    kfree_obj(*p);
}

// ---- Allocation failure ----

void test_kalloc_fail()
{
    reset_mock_state();
    __mock_kmalloc_fail = 1;
    auto p = kalloc<TestObj>(99);
    assert(!p.has_value());
    assert(p.error() == ErrorCode::AllocFail);
}

void test_kalloc_array_fail()
{
    reset_mock_state();
    __mock_kmalloc_fail = 1;
    auto arr = kalloc_array<int>(4);
    assert(!arr.has_value());
    assert(arr.error() == ErrorCode::AllocFail);
}

void test_kalloc_array_overflow_guard()
{
    reset_mock_state();
    auto arr = kalloc_array<int>(static_cast<size_t>(-1));
    assert(!arr.has_value());
    assert(arr.error() == ErrorCode::AllocFail);
}

// ---- kfree_obj ----

void test_kfree_obj_null()
{
    TestObj* p = nullptr;
    kfree_obj(p); // Must be a silent no-op
}

void test_kfree_obj_calls_destructor()
{
    reset_mock_state();
    TestObj::destructor_count = 0;
    auto result = kalloc<TestObj>(7);
    assert(result.has_value());
    TestObj* raw = *result;
    assert(raw->x == 7);
    kfree_obj(raw);
    assert(TestObj::destructor_count == 1);
}

// ---- Array allocation ----

void test_kalloc_array_success()
{
    reset_mock_state();
    auto arr = kalloc_array<int>(10);
    assert(arr.has_value());
    int* p = *arr;
    for (int i = 0; i < 10; ++i)
        p[i] = i;
    cpp_kfree(p);
}

void test_gfp_priority_with_multiple_signals()
{
    reset_mock_state();
    __mock_preempt_count = 1;
    __mock_irqs_disabled = 1;
    __mock_in_nmi = 1;
    auto p = kalloc<TestObj>(5);
    assert(p.has_value());
    assert(__mock_last_gfp == CPP_GFP_ATOMIC);
    kfree_obj(*p);
}

extern "C" int main()
{
    test_kalloc_success();
    test_kalloc_atomic();
    test_kalloc_irqs_disabled();
    test_kalloc_nmi();
    test_gfp_priority_with_multiple_signals();
    test_kalloc_fail();
    test_kalloc_array_fail();
    test_kalloc_array_overflow_guard();
    test_kfree_obj_null();
    test_kfree_obj_calls_destructor();
    test_kalloc_array_success();
    printf("All allocator tests passed!\n");
    return 0;
}
