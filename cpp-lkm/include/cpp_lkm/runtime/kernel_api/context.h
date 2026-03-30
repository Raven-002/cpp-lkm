#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    int cpp_in_atomic(void);
    int cpp_irqs_disabled(void);
    int cpp_in_nmi(void);

#ifdef __cplusplus
}
#endif
