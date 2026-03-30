#pragma once

#include "cpp_lkm/runtime/kernel_api/types.h"

#ifdef __cplusplus
#include <cstdint>

using cpp_ssize_t = std::int64_t;
using cpp_chardev_read_cb = cpp_ssize_t (*)(void* ctx, void* kbuf, size_t len, std::int64_t* pos);
using cpp_chardev_write_cb = cpp_ssize_t (*)(void* ctx, const void* kbuf, size_t len,
                                             std::int64_t* pos);
#else
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

typedef int64_t cpp_ssize_t;
typedef cpp_ssize_t (*cpp_chardev_read_cb)(void* ctx, void* kbuf, size_t len, int64_t* pos);
typedef cpp_ssize_t (*cpp_chardev_write_cb)(void* ctx, const void* kbuf, size_t len, int64_t* pos);
#endif

/* Adjacent string literal concatenation at call sites requires string-literal macros. */
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define CPP_KERN_INFO "[INFO] "
#define CPP_KERN_ERR "[ERR]  "
// NOLINTEND(cppcoreguidelines-macro-usage)
