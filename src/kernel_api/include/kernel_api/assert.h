#pragma once

#include "kernel_api/types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void cpp_assert_fail(const char* expr, const char* file, int line, const char* func);

#ifdef __cplusplus
}
#endif
