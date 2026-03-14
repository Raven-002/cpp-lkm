#include <linux/module.h>
#include <linux/slab.h>
#include <linux/preempt.h>
#include <linux/hardirq.h>
#include <linux/irqflags.h>

// This file is compiled by Kbuild to provide the necessary 
// kernel module metadata and entry points.

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Antigravity Agent");
MODULE_DESCRIPTION("C++ Kernel Module via CMake+Kbuild");

// --- C++ Bridge Implementations ---

int cpp_printk(const char *fmt, ...) {
    va_list args;
    int r;
    va_start(args, fmt);
    r = vprintk(fmt, args);
    va_end(args);
    return r;
}

void *cpp_kmalloc(size_t size, gfp_t flags) {
    return kmalloc(size, flags);
}

void cpp_kfree(const void *p) {
    kfree(p);
}

void cpp_assert_fail(const char *expr, const char *file, int line, const char *func) {
    panic("C++ BUG(): %s at %s:%d %s", expr, file, line, func);
}

/* Resolve C++ assert() from e.g. tl::expected TL_ASSERT; kernel has no libc __assert_fail */
void __assert_fail(const char *assertion, const char *file, int line, const char *function) {
    cpp_assert_fail(assertion, file, line, function);
}

int cpp_in_atomic(void) {
    return in_atomic();
}

int cpp_irqs_disabled(void) {
    return irqs_disabled();
}

int cpp_in_nmi(void) {
    return in_nmi();
}

// --- Module lifecycle ---

// These will be resolved to the C++ entry points in bridge.cpp
extern int  cpp_module_init(void);
extern void cpp_module_exit(void);

static int __init lkm_init(void) {
    return cpp_module_init();
}

static void __exit lkm_exit(void) {
    cpp_module_exit();
}

module_init(lkm_init);
module_exit(lkm_exit);
