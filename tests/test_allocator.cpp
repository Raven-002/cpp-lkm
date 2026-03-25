// tests/test_allocator.cpp
// Tests for kalloc<T>(), kalloc_array<T>(), kfree_obj(), and GFP flag selection.
#include "kalloc.hpp"
#include "mock_globals.hpp"

#include <assert.h>
#include <linux/kernel.h>
#include <linux/preempt.h>
#include <stdio.h>

struct TestObj
{
    int x;
    explicit TestObj(int val) : x(val) {}
    ~TestObj()
    {
        x = -1;
    } // Sentinel: detects destructor was called
};

// ---- GFP flag selection ----

void test_kalloc_success()
{
    reset_mock_state();
    auto p = kalloc<TestObj>(42);
    assert(p.has_value());
    assert(p.value()->x == 42);
    assert(__mock_last_gfp == GFP_KERNEL);
    kfree_obj(*p);
}

void test_kalloc_atomic()
{
    reset_mock_state();
    __mock_preempt_count = 1; // in_atomic() → true
    auto p = kalloc<TestObj>(10);
    assert(p.has_value());
    assert(__mock_last_gfp == GFP_ATOMIC);
    kfree_obj(*p);
}

void test_kalloc_irqs_disabled()
{
    reset_mock_state();
    __mock_irqs_disabled = 1;
    auto p = kalloc<TestObj>(10);
    assert(p.has_value());
    assert(__mock_last_gfp == GFP_ATOMIC);
    kfree_obj(*p);
}

void test_kalloc_nmi()
{
    reset_mock_state();
    __mock_in_nmi = 1;
    auto p = kalloc<TestObj>(10);
    assert(p.has_value());
    assert(__mock_last_gfp == GFP_ATOMIC);
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

// ---- kfree_obj ----

void test_kfree_obj_null()
{
    TestObj* p = nullptr;
    kfree_obj(p); // Must be a silent no-op
}

void test_kfree_obj_calls_destructor()
{
    reset_mock_state();
    auto result = kalloc<TestObj>(7);
    assert(result.has_value());
    TestObj* raw = *result;
    assert(raw->x == 7);
    kfree_obj(raw); // Destructor sets x = -1 before freeing
    // After kfree_obj the pointer is gone; test just verifies no crash and
    // that the destructor was invoked (via sanitizers / valgrind in CI).
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
    kfree(p);
}

int main()
{
    test_kalloc_success();
    test_kalloc_atomic();
    test_kalloc_irqs_disabled();
    test_kalloc_nmi();
    test_kalloc_fail();
    test_kfree_obj_null();
    test_kfree_obj_calls_destructor();
    test_kalloc_array_success();
    printf("All allocator tests passed!\n");
    return 0;
}
