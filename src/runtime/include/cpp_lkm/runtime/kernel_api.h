#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
#include <cstdint>

using cpp_ssize_t = std::int64_t;
using cpp_chardev_read_cb = cpp_ssize_t (*)(void* ctx, void* kbuf, size_t len, std::int64_t* pos);
using cpp_chardev_write_cb = cpp_ssize_t (*)(void* ctx, const void* kbuf, size_t len,
                                             std::int64_t* pos);

using cpp_gfp_t = unsigned int;
inline constexpr cpp_gfp_t CPP_GFP_KERNEL = 0x1U;
inline constexpr cpp_gfp_t CPP_GFP_ATOMIC = 0x2U;
#else
typedef int64_t cpp_ssize_t;
typedef cpp_ssize_t (*cpp_chardev_read_cb)(void* ctx, void* kbuf, size_t len, int64_t* pos);
typedef cpp_ssize_t (*cpp_chardev_write_cb)(void* ctx, const void* kbuf, size_t len, int64_t* pos);

typedef unsigned int cpp_gfp_t;
#define CPP_GFP_KERNEL 0x1U
#define CPP_GFP_ATOMIC 0x2U
#endif

/* Adjacent string literal concatenation at call sites requires string-literal macros. */
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define CPP_KERN_INFO "[INFO] "
#define CPP_KERN_ERR "[ERR]  "
// NOLINTEND(cppcoreguidelines-macro-usage)

#ifdef __cplusplus
extern "C"
{
#endif

    int cpp_printk(const char* fmt, ...);
    void* cpp_kmalloc(size_t size, cpp_gfp_t flags);
    void cpp_kfree(const void* p);
    void cpp_assert_fail(const char* expr, const char* file, int line, const char* func);
    int cpp_in_atomic(void);
    int cpp_irqs_disabled(void);
    int cpp_in_nmi(void);

    /* Character device (miscdevice) bridge; see linux_bridge.c / mock_kernel_bridge.cpp */
    int cpp_userspace_chardev_register(const char* name, unsigned int mode, void* ctx,
                                       cpp_chardev_read_cb read_cb, cpp_chardev_write_cb write_cb);
    void cpp_userspace_chardev_unregister(void);

#ifdef __cplusplus
}
#endif
