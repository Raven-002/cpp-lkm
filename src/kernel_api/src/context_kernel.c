#include <linux/hardirq.h>
#include <linux/preempt.h>

#include "cpp_lkm/runtime/kernel_api.h"

int cpp_in_atomic(void)
{
    return in_atomic() ? 1 : 0;
}

int cpp_irqs_disabled(void)
{
    return irqs_disabled() ? 1 : 0;
}

int cpp_in_nmi(void)
{
    return in_nmi() ? 1 : 0;
}
