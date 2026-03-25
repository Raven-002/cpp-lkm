// Declares mock state variables and the reset helper used by all test suites.
//
// How mock control works:
//   __mock_kmalloc_fail      - next kmalloc call returns NULL (then auto-resets)
//   __mock_kmalloc_fail_after - Nth call to cpp_kmalloc returns NULL
//   __mock_preempt_count     - non-zero -> cpp_in_atomic() returns true
//   __mock_irqs_disabled     - non-zero -> cpp_irqs_disabled() returns true
//   __mock_in_nmi            - non-zero -> cpp_in_nmi() returns true
//   __mock_cpp_constructed_count/__mock_cpp_initialized_count/__mock_cpp_destructed_count
//                            - message counters extracted from cpp_printk output
#pragma once

#include "cpp_lkm/runtime/kernel_api.h"

extern "C"
{
    extern cpp_gfp_t __mock_last_gfp;
    extern int __mock_kmalloc_fail;
    // If set to N > 0, the Nth call to cpp_kmalloc will fail (then resets to 0).
    extern int __mock_kmalloc_fail_after;
    extern int __mock_preempt_count;
    extern int __mock_irqs_disabled;
    extern int __mock_in_nmi;
    extern int __mock_cpp_constructed_count;
    extern int __mock_cpp_initialized_count;
    extern int __mock_cpp_destructed_count;
}

// Call at the start of every test to ensure clean mock state.
inline void reset_mock_state() noexcept
{
    __mock_last_gfp = 0;
    __mock_kmalloc_fail = 0;
    __mock_kmalloc_fail_after = 0;
    __mock_preempt_count = 0;
    __mock_irqs_disabled = 0;
    __mock_in_nmi = 0;
    __mock_cpp_constructed_count = 0;
    __mock_cpp_initialized_count = 0;
    __mock_cpp_destructed_count = 0;
}
