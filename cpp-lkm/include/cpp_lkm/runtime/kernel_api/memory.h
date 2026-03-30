#pragma once

#include "cpp_lkm/runtime/kernel_api/types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void* cpp_kmalloc(size_t size, cpp_gfp_t flags);
    void cpp_kfree(const void* ptr);

#ifdef __cplusplus
}
#endif
