#pragma once

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>

using cpp_ssize_t = std::int64_t;
using cpp_chardev_read_cb = cpp_ssize_t (*)(void* ctx, void* kbuf, size_t len, std::int64_t* pos);
using cpp_chardev_write_cb = cpp_ssize_t (*)(void* ctx, const void* kbuf, size_t len,
                                             std::int64_t* pos);

using cpp_gfp_t = unsigned int;
inline constexpr cpp_gfp_t CPP_GFP_KERNEL = 0x1U;
inline constexpr cpp_gfp_t CPP_GFP_ATOMIC = 0x2U;
#else
#ifdef __KERNEL__
#include <linux/stddef.h>
#include <linux/types.h>
#else
#include <stddef.h>
#include <stdint.h>
#endif

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
