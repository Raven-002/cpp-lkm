#include <linux/hardirq.h>
#include <linux/preempt.h>
#include <stdbool.h>

#include "kernel_api/kernel_api.h"

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
