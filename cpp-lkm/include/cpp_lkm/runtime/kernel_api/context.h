#pragma once

#ifndef __cplusplus
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdbool.h>
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    bool cpp_in_atomic(void);
    bool cpp_irqs_disabled(void);
    bool cpp_in_nmi(void);

#ifdef __cplusplus
}
#endif
