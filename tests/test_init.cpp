// tests/test_init.cpp
// Tests for CppKernelModule two-phase initialization and cleanup.
#include "error.hpp"
#include "mock_globals.hpp"

#include <assert.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/preempt.h>
#include <stdio.h>

void test_happy_path()
{
    reset_mock_state();
    int res = cpp_module_init();
    assert(res == 0);
    assert(__mock_cpp_constructed_count == 1);
    assert(__mock_cpp_initialized_count == 1);
    assert(__mock_cpp_destructed_count == 0);
    cpp_module_exit();
    assert(__mock_cpp_destructed_count == 1);
}

void test_alloc_fail_in_init()
{
    reset_mock_state();
    __mock_kmalloc_fail = 1;
    int res = cpp_module_init();
    assert(res == to_errno(ErrorCode::AllocFail));
    assert(__mock_cpp_constructed_count == 1);
    assert(__mock_cpp_initialized_count == 0);
    assert(__mock_cpp_destructed_count == 1);
    cpp_module_exit(); // Must be safe even after failed init
    assert(__mock_cpp_destructed_count == 1);
}

void test_partial_init_cleanup()
{
    reset_mock_state();
    __mock_kmalloc_fail_after = 2; // Second allocation (resource2) fails
    int res = cpp_module_init();
    assert(res == to_errno(ErrorCode::AllocFail));
    assert(__mock_cpp_constructed_count == 1);
    assert(__mock_cpp_initialized_count == 0);
    assert(__mock_cpp_destructed_count == 1);
    // Module instance was torn down; exit must still be safe
    cpp_module_exit();
    assert(__mock_cpp_destructed_count == 1);
}

void test_exit_after_failed_init()
{
    reset_mock_state();
    __mock_kmalloc_fail = 1;
    cpp_module_init();
    // g_module is null here; cpp_module_exit must be a no-op
    cpp_module_exit();
    assert(__mock_cpp_destructed_count == 1);
}

void test_destructor_print()
{
    reset_mock_state();
    assert(cpp_module_init() == 0);
    cpp_module_exit();
    assert(__mock_cpp_destructed_count == 1);
}

void test_errno_mapping()
{
    assert(to_errno(ErrorCode::None) == 0);
    assert(to_errno(ErrorCode::AllocFail) == -12);
}

int main()
{
    test_happy_path();
    test_alloc_fail_in_init();
    test_partial_init_cleanup();
    test_exit_after_failed_init();
    test_destructor_print();
    test_errno_mapping();
    printf("All init tests passed!\n");
    return 0;
}
