#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int __mock_preempt_count;
extern int __mock_irqs_disabled;
extern int __mock_in_nmi;

static inline int in_atomic(void)      { return __mock_preempt_count != 0; }
static inline int irqs_disabled(void)  { return __mock_irqs_disabled != 0; }
static inline int in_nmi(void)         { return __mock_in_nmi != 0; }

#ifdef __cplusplus
}
#endif
