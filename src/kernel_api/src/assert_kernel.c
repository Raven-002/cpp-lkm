#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/processor.h>

#include "cpp_lkm/runtime/kernel_api.h"

void cpp_assert_fail(const char* expr, const char* file, int line, const char* func)
{
    pr_err("[CPP] assertion failed: %s at %s:%d in %s\n", expr, file, line, func);
    for (;;)
    {
        cpu_relax();
    }
}
