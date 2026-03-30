#include <linux/hardirq.h>
#include <linux/preempt.h>
#include <linux/types.h>

#include "cpp_lkm/runtime/kernel_api/context.h"

bool cpp_in_atomic(void)
{
    return in_atomic();
}

bool cpp_irqs_disabled(void)
{
    return irqs_disabled();
}

bool cpp_in_nmi(void)
{
    return in_nmi();
}
