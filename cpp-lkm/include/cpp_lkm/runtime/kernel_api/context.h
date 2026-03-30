#pragma once

#ifndef __cplusplus
#include <stdbool.h>
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
