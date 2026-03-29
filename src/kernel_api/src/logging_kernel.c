#include <linux/kernel.h>
#include <linux/printk.h>

#include "cpp_lkm/runtime/kernel_api.h"

int cpp_printk(const char* fmt, ...)
{
    va_list args;
    int ret = 0;

    va_start(args, fmt);
    ret = vprintk(fmt, args);
    va_end(args);
    return ret;
}
