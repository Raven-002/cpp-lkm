#include <linux/slab.h>

#include "cpp_lkm/runtime/kernel_api/memory.h"

enum
{
    CPP_GFP_ATOMIC_VAL = 0x2U
};

void* cpp_kmalloc(size_t size, cpp_gfp_t flags)
{
    const gfp_t gfp = ((flags & CPP_GFP_ATOMIC_VAL) != 0U) ? GFP_ATOMIC : GFP_KERNEL;
    return kmalloc(size, gfp);
}

void cpp_kfree(const void* ptr)
{
    kfree(ptr);
}
