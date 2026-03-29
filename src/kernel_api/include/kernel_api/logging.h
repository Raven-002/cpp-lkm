#pragma once

#include "kernel_api/types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int cpp_printk(const char* fmt, ...);

#ifdef __cplusplus
}
#endif
