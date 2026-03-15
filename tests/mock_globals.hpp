#pragma once
#include <linux/kernel.h>
#include <linux/preempt.h>

extern "C"
{
    extern gfp_t __mock_last_gfp;
    extern int __mock_kmalloc_fail;
    // If set to N > 0, the Nth call to cpp_kmalloc will fail (and then reset to 0).
    extern int __mock_kmalloc_fail_after;
    extern int __mock_preempt_count;
    extern int __mock_irqs_disabled;
    extern int __mock_in_nmi;
}
