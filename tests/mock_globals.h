#pragma once
#include <linux/kernel.h>
#include <linux/preempt.h>

extern "C" {
    extern gfp_t __mock_last_gfp;
    extern int   __mock_kmalloc_fail;
    extern int   __mock_preempt_count;
    extern int   __mock_irqs_disabled;
    extern int   __mock_in_nmi;
}
