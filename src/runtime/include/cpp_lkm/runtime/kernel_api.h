#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned int cpp_gfp_t;

#define CPP_GFP_KERNEL 0x1u
#define CPP_GFP_ATOMIC 0x2u

#define CPP_KERN_INFO "[INFO] "
#define CPP_KERN_ERR "[ERR]  "

int cpp_printk(const char* fmt, ...);
void* cpp_kmalloc(size_t size, cpp_gfp_t flags);
void cpp_kfree(const void* p);
void cpp_assert_fail(const char* expr, const char* file, int line, const char* func);
int cpp_in_atomic(void);
int cpp_irqs_disabled(void);
int cpp_in_nmi(void);

#ifdef __cplusplus
}
#endif
