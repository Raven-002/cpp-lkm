#pragma once

#include "kernel_api/types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Character device (miscdevice) bridge; consumer kernel_api + mock_kernel_bridge.cpp */
    int cpp_userspace_chardev_register(const char* name, unsigned int mode, void* ctx,
                                       cpp_chardev_read_cb read_cb, cpp_chardev_write_cb write_cb);
    void cpp_userspace_chardev_unregister(void* ctx);

#ifdef __cplusplus
}
#endif
