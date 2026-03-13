#pragma once
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define printk(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define KERN_INFO  "[INFO] "
#define KERN_ERR   "[ERR]  "

typedef unsigned int gfp_t;
#define GFP_KERNEL  0x1u
#define GFP_ATOMIC  0x2u

/* Inspectable by test_allocator.cpp to verify GFP flag selection */
extern gfp_t __mock_last_gfp;
/* Set to true to make the next kmalloc call return null */
extern int   __mock_kmalloc_fail;

static inline void* kmalloc(size_t size, gfp_t flags) {
    __mock_last_gfp = flags;
    if (__mock_kmalloc_fail) { __mock_kmalloc_fail = 0; return NULL; }
    return __builtin_malloc(size);
}
static inline void kfree(void* p) { __builtin_free(p); }

#define __init
#define __exit

/* BUG() defined for visibility in tests — must never appear in src/ or include/ */
#define BUG() \
    do { fprintf(stderr, "BUG() called at %s:%d\n", __FILE__, __LINE__); __builtin_abort(); } while(0)
#define BUG_ON(cond) do { if (cond) BUG(); } while(0)

#ifdef __cplusplus
}
#endif
