// Declares mock state variables and the reset helper used by all test suites.
//
// How mock control works:
//   g_mock_kmalloc_fail       - next kmalloc call returns NULL (then auto-resets)
//   g_mock_kmalloc_fail_after - Nth call to cpp_kmalloc returns NULL
//   g_mock_preempt_count      - non-zero -> cpp_in_atomic() returns true
//   g_mock_irqs_disabled      - non-zero -> cpp_irqs_disabled() returns true
//   g_mock_in_nmi             - non-zero -> cpp_in_nmi() returns true
//   g_mock_cpp_*_count        - message counters extracted from cpp_printk output
//   g_mock_chardev_registered - set by mock cpp_userspace_chardev_register/unregister
//   g_mock_chardev_reg_fail   - next register returns failure (then resets to 0)
#pragma once

#include "cpp_lkm/runtime/kernel_api.h"

extern "C"
{
    extern cpp_gfp_t g_mock_last_gfp;
    extern int g_mock_kmalloc_fail;
    // If set to N > 0, the Nth call to cpp_kmalloc will fail (then resets to 0).
    extern int g_mock_kmalloc_fail_after;
    extern int g_mock_preempt_count;
    extern int g_mock_irqs_disabled;
    extern int g_mock_in_nmi;
    extern int g_mock_cpp_constructed_count;
    extern int g_mock_cpp_initialized_count;
    extern int g_mock_cpp_destructed_count;
    extern int g_mock_chardev_registered;
    extern int g_mock_chardev_reg_fail;
}

// Call at the start of every test to ensure clean mock state.
inline void reset_mock_state() noexcept
{
    g_mock_last_gfp = 0;
    g_mock_kmalloc_fail = 0;
    g_mock_kmalloc_fail_after = 0;
    g_mock_preempt_count = 0;
    g_mock_irqs_disabled = 0;
    g_mock_in_nmi = 0;
    g_mock_cpp_constructed_count = 0;
    g_mock_cpp_initialized_count = 0;
    g_mock_cpp_destructed_count = 0;
    g_mock_chardev_registered = 0;
    g_mock_chardev_reg_fail = 0;
}
