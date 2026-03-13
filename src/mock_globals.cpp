#include <linux/kernel.h>
#include <linux/preempt.h>

extern "C" {
    gfp_t __mock_last_gfp = 0;
    int   __mock_kmalloc_fail = 0;
    int   __mock_preempt_count = 0;
    int   __mock_irqs_disabled = 0;
    int   __mock_in_nmi = 0;
}
