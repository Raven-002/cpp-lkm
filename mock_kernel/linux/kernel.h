#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int gfp_t;
#define GFP_KERNEL  0x1u
#define GFP_ATOMIC  0x2u

#define KERN_INFO  "[INFO] "
#define KERN_ERR   "[ERR]  "

// Use bridge functions to prevent symbol clashes when linking with the real kernel
extern int cpp_printk(const char *fmt, ...);
#define printk(...) cpp_printk(__VA_ARGS__)

extern void *cpp_kmalloc(size_t size, gfp_t flags);
#define kmalloc(size, flags) cpp_kmalloc(size, flags)

extern void cpp_kfree(const void *p);
#define kfree(p) cpp_kfree(p)

extern void cpp_assert_fail(const char *expr, const char *file, int line, const char *func);
#define BUG() do { cpp_assert_fail("BUG()", __FILE__, __LINE__, __func__); } while(0)
#define BUG_ON(cond) do { if (cond) BUG(); } while(0)

#define __init
#define __exit

#ifdef __cplusplus
}
#endif
