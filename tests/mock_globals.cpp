#include "mock_globals.hpp"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

extern "C"
{
    // Allocation tracking
    gfp_t __mock_last_gfp = 0;
    int __mock_kmalloc_fail = 0;
    int __mock_kmalloc_fail_after = 0;

    // Context tracking
    int __mock_preempt_count = 0;
    int __mock_irqs_disabled = 0;
    int __mock_in_nmi = 0;

    // Mock implementations for host mode via bridged symbols
    int cpp_printk(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        int res = vprintf(fmt, args);
        va_end(args);
        return res;
    }

    void* cpp_kmalloc(size_t size, gfp_t flags)
    {
        __mock_last_gfp = flags;
        if (__mock_kmalloc_fail_after > 0)
        {
            --__mock_kmalloc_fail_after;
            if (__mock_kmalloc_fail_after == 0)
            {
                return NULL;
            }
        }
        if (__mock_kmalloc_fail)
        {
            __mock_kmalloc_fail = 0;
            return NULL;
        }
        return __builtin_malloc(size);
    }

    void cpp_kfree(const void* p)
    {
        __builtin_free((void*)p);
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
