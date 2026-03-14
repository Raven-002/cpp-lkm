#include "kalloc.h"
#include <linux/kernel.h>
#include <linux/preempt.h>
#include <stdio.h>
#include <assert.h>
#include "mock_globals.h"

struct TestObj {
    int x;
    TestObj(int val) : x(val) {}
};

void test_kalloc_success() {
    __mock_preempt_count = 0;
    __mock_irqs_disabled = 0;
    __mock_in_nmi = 0;
    __mock_kmalloc_fail = 0;
    __mock_kmalloc_fail_after = 0;

    auto p = kalloc<TestObj>(42);
    assert(p.has_value());
    assert(p.value()->x == 42);
    assert(__mock_last_gfp == GFP_KERNEL);

    kfree_obj(*p);
}

void test_kalloc_atomic() {
    __mock_preempt_count = 1;
    __mock_kmalloc_fail_after = 0;

    auto p = kalloc<TestObj>(10);
    assert(p.has_value());
    assert(__mock_last_gfp == GFP_ATOMIC);

    kfree_obj(*p);
    __mock_preempt_count = 0;
}

void test_kalloc_fail() {
    __mock_kmalloc_fail = 1;
    __mock_kmalloc_fail_after = 0;
    auto p = kalloc<TestObj>(99);
    assert(!p.has_value());
    assert(p.error() == ErrorCode::AllocFail);
}

void test_kalloc_array_success() {
    __mock_kmalloc_fail = 0;
    __mock_preempt_count = 0;
    __mock_kmalloc_fail_after = 0;
    
    auto arr = kalloc_array<int>(10);
    assert(arr.has_value());
    
    int* p = *arr;
    for (int i=0; i<10; ++i) p[i] = i;

    kfree(p); // kfree_obj isn't for arrays typically, but kalloc_array returns T*
}

void test_kfree_obj_null() {
    TestObj* p = nullptr;
    kfree_obj(p); // Should not crash
}

int main() {
    test_kalloc_success();
    test_kalloc_atomic();
    test_kalloc_fail();
    test_kalloc_array_success();
    test_kfree_obj_null();
    printf("All allocator tests passed!\n");
    return 0;
}
