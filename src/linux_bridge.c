#include <linux/module.h>

// This file is compiled by Kbuild to provide the necessary 
// kernel module metadata and entry points.

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Antigravity Agent");
MODULE_DESCRIPTION("C++ Kernel Module via CMake+Kbuild");

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
