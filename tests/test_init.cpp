// tests/test_init.cpp
// Tests for CppKernelModule two-phase initialization and cleanup.
#include "cpp_lkm/runtime/module_entry.h"
#include "tests/support/mock_globals.hpp"

#include <cassert>
#include <cstdio>

static void test_happy_path()
{
    reset_mock_state();
    const int res = cpp_module_init();
    assert(res == 0);
    assert(g_mock_cpp_constructed_count == 1);
    assert(g_mock_cpp_initialized_count == 1);
    assert(g_mock_cpp_destructed_count == 0);
    assert(g_mock_chardev_registered == 1);
    cpp_module_exit();
    assert(g_mock_cpp_destructed_count == 1);
    assert(g_mock_chardev_registered == 0);
}

static void test_destructor_print()
{
    reset_mock_state();
    assert(cpp_module_init() == 0);
    cpp_module_exit();
    assert(g_mock_cpp_destructed_count == 1);
}

extern "C" int main()
{
    test_happy_path();
    test_destructor_print();
    printf("All init tests passed!\n");
    return 0;
}
