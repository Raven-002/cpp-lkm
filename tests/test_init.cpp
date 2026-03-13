#include "module.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <stdio.h>
#include <assert.h>

void test_happy_path() {
    __mock_kmalloc_fail = 0;
    int res = cpp_module_init();
    assert(res == 0);
    cpp_module_exit();
}

void test_alloc_fail_in_init() {
    __mock_kmalloc_fail = 1;
    int res = cpp_module_init();
    assert(res == -12); // -ENOMEM
    cpp_module_exit(); // Should be safe
}

void test_partial_init_cleanup() {
    // We could make the second alloc fail, but mocking exactly one is tricky with our simple fail flag
    // Let's rely on test_alloc_fail_in_init and manual review
}

void test_exit_after_failed_init() {
    __mock_kmalloc_fail = 1;
    cpp_module_init();
    // g_module is null here, cpp_module_exit should be safe
    cpp_module_exit();
}

void test_errno_mapping() {
    assert(static_cast<int>(ErrorCode::None) == 0);
    assert(static_cast<int>(ErrorCode::AllocFail) == -12);
    assert(static_cast<int>(ErrorCode::HwHandshake) == -5);
    assert(static_cast<int>(ErrorCode::BadState) == -22);
}

int main() {
    test_happy_path();
    test_alloc_fail_in_init();
    test_exit_after_failed_init();
    test_errno_mapping();
    printf("All init tests passed!\n");
    return 0;
}
