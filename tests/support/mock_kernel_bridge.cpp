// Host-mode implementations of the C bridge functions declared in runtime headers.
//
// This is the host-mode counterpart to src/runtime/linux_bridge.c:
//   linux_bridge.c       - Kbuild: wraps real kernel APIs (printk, kmalloc, ...)
//   mock_kernel_bridge.cpp - Host tests: wraps libc + mock state for the same symbols
//
// Adding a new kernel API bridge:
//   1. Declare the cpp_* function in include/cpp_lkm/runtime/kernel_api.h.
//   2. Implement the real wrapper in src/runtime/linux_bridge.c.
//   3. Implement the mock wrapper here.
#include "tests/support/mock_globals.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
    // ----- Allocation state (definitions) -----
    cpp_gfp_t __mock_last_gfp = 0;
    int __mock_kmalloc_fail = 0;
    int __mock_kmalloc_fail_after = 0;

    // ----- CPU context state (definitions) -----
    int __mock_preempt_count = 0;
    int __mock_irqs_disabled = 0;
    int __mock_in_nmi = 0;
    int __mock_cpp_constructed_count = 0;
    int __mock_cpp_initialized_count = 0;
    int __mock_cpp_destructed_count = 0;

    // ----- Bridge implementations -----

    int cpp_printk(const char* fmt, ...)
    {
        if (strstr(fmt, "[CPP] Constructed") != nullptr)
            ++__mock_cpp_constructed_count;
        if (strstr(fmt, "[CPP] Initialized") != nullptr)
            ++__mock_cpp_initialized_count;
        if (strstr(fmt, "[CPP] Destructed") != nullptr)
            ++__mock_cpp_destructed_count;

        va_list args;
        va_start(args, fmt);
        int res = vprintf(fmt, args);
        va_end(args);
        return res;
    }

    void* cpp_kmalloc(size_t size, cpp_gfp_t flags)
    {
        __mock_last_gfp = flags;

        if (__mock_kmalloc_fail_after > 0)
        {
            --__mock_kmalloc_fail_after;
            if (__mock_kmalloc_fail_after == 0)
                return nullptr;
        }

        if (__mock_kmalloc_fail)
        {
            __mock_kmalloc_fail = 0;
            return nullptr;
        }

        return __builtin_malloc(size);
    }

    void cpp_kfree(const void* p)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        __builtin_free(const_cast<void*>(p));
    }

    void cpp_assert_fail(const char* expr, const char* file, int line, const char* func)
    {
        fprintf(stderr, "BUG() triggered: %s at %s:%d in %s\n", expr, file, line, func);
        abort();
    }

    int cpp_in_atomic(void)
    {
        return __mock_preempt_count != 0;
    }

    int cpp_irqs_disabled(void)
    {
        return __mock_irqs_disabled != 0;
    }

    int cpp_in_nmi(void)
    {
        return __mock_in_nmi != 0;
    }
}
