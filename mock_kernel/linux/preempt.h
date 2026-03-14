#pragma once
#ifdef __cplusplus
extern "C" {
#endif

extern int cpp_in_atomic(void);
extern int cpp_irqs_disabled(void);
extern int cpp_in_nmi(void);

#define in_atomic()     cpp_in_atomic()
#define irqs_disabled() cpp_irqs_disabled()
#define in_nmi()        cpp_in_nmi()

#ifdef __cplusplus
}
#endif
