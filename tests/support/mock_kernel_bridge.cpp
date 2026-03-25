// Host-mode implementations of the C bridge functions declared in runtime headers.
//
// This is the host-mode counterpart to src/runtime/linux_bridge.c:
//   linux_bridge.c       - Kbuild: wraps real kernel APIs (printk, kmalloc, ...)
//   mock_kernel_bridge.cpp - Host tests: wraps libc + mock state for the same symbols
//
// Adding a new kernel API bridge:
//   1. Declare the cpp_* function in include/cpp_lkm/runtime/kernel_api.h.
//   2. Implement the real wrapper in src/runtime/linux_bridge.c.
//   3. Implement the mock wrapper here.
#include "tests/support/mock_globals.hpp"

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{
cpp_chardev_read_cb g_chardev_read_cb = nullptr;
cpp_chardev_write_cb g_chardev_write_cb = nullptr;
void* g_chardev_ctx = nullptr;
} // namespace

extern "C"
{
    // ----- Allocation state (definitions) -----
    cpp_gfp_t g_mock_last_gfp = 0;
    int g_mock_kmalloc_fail = 0;
    int g_mock_kmalloc_fail_after = 0;

    // ----- CPU context state (definitions) -----
    int g_mock_preempt_count = 0;
    int g_mock_irqs_disabled = 0;
    int g_mock_in_nmi = 0;
    int g_mock_cpp_constructed_count = 0;
    int g_mock_cpp_initialized_count = 0;
    int g_mock_cpp_destructed_count = 0;
    int g_mock_chardev_registered = 0;
    int g_mock_chardev_reg_fail = 0;

    // ----- Bridge implementations -----

    int cpp_printk(const char* fmt, ...)
    {
        if (strstr(fmt, "[CPP] Constructed") != nullptr)
        {
            ++g_mock_cpp_constructed_count;
        }
        if (strstr(fmt, "[CPP] Initialized") != nullptr)
        {
            ++g_mock_cpp_initialized_count;
        }
        if (strstr(fmt, "[CPP] Destructed") != nullptr)
        {
            ++g_mock_cpp_destructed_count;
        }

        va_list args;
        va_start(args, fmt);
        const int res = vprintf(fmt, args);
        va_end(args);
        return res;
    }

    void* cpp_kmalloc(size_t size, cpp_gfp_t flags)
    {
        g_mock_last_gfp = flags;

        if (g_mock_kmalloc_fail_after > 0)
        {
            --g_mock_kmalloc_fail_after;
            if (g_mock_kmalloc_fail_after == 0)
            {
                return nullptr;
            }
        }

        if (g_mock_kmalloc_fail != 0)
        {
            g_mock_kmalloc_fail = 0;
            return nullptr;
        }

        return __builtin_malloc(size);
    }

    void cpp_kfree(const void* ptr)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        __builtin_free(const_cast<void*>(ptr));
    }

    void cpp_assert_fail(const char* expr, const char* file, int line, const char* func)
    {
        (void)fprintf(stderr, "BUG() triggered: %s at %s:%d in %s\n", expr, file, line, func);
        std::abort();
    }

    int cpp_in_atomic(void)
    {
        return static_cast<int>(g_mock_preempt_count != 0);
    }

    int cpp_irqs_disabled(void)
    {
        return static_cast<int>(g_mock_irqs_disabled != 0);
    }

    int cpp_in_nmi(void)
    {
        return static_cast<int>(g_mock_in_nmi != 0);
    }

    int cpp_userspace_chardev_register(const char* name, unsigned int mode, void* ctx,
                                       cpp_chardev_read_cb read_cb, cpp_chardev_write_cb write_cb)
    {
        (void)name;
        (void)mode;
        if (g_mock_chardev_reg_fail != 0)
        {
            g_mock_chardev_reg_fail = 0;
            return -5; /* EIO */
        }
        g_chardev_ctx = ctx;
        g_chardev_read_cb = read_cb;
        g_chardev_write_cb = write_cb;
        g_mock_chardev_registered = 1;
        return 0;
    }

    void cpp_userspace_chardev_unregister(void)
    {
        g_chardev_ctx = nullptr;
        g_chardev_read_cb = nullptr;
        g_chardev_write_cb = nullptr;
        g_mock_chardev_registered = 0;
    }

    cpp_ssize_t cpp_mock_chardev_simulate_read(void* kbuf, size_t len, std::int64_t* pos)
    {
        if (g_chardev_read_cb == nullptr || g_chardev_ctx == nullptr)
        {
            return -22;
        }
        return g_chardev_read_cb(g_chardev_ctx, kbuf, len, pos);
    }

    cpp_ssize_t cpp_mock_chardev_simulate_write(const void* kbuf, size_t len, std::int64_t* pos)
    {
        if (g_chardev_write_cb == nullptr || g_chardev_ctx == nullptr)
        {
            return -22;
        }
        return g_chardev_write_cb(g_chardev_ctx, kbuf, len, pos);
    }
}
