// Host-mode implementations of the C bridge functions declared under
// src/kernel_api/include/kernel_api/.
//
// Host-mode counterpart to src/kernel_api/src/*.c (Kbuild-compiled for the real .ko).
//
// Adding a new kernel API bridge:
//   1. Declare the cpp_* function in src/kernel_api/include/kernel_api/*.h (and umbrella if
//   needed).
//   2. Implement the real wrapper in src/kernel_api/src/*.c (extern "C" exports).
//   3. Implement the mock wrapper here.
#include "kernel_api/chardev.h"

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{
constexpr size_t k_max_mock_chardev_slots = 8;
constexpr size_t k_max_mock_name_len = 63;

struct MockCharDevSlot
{
    char name[k_max_mock_name_len + 1U]{};
    size_t name_len = 0;
    void* ctx = nullptr;
    cpp_chardev_read_cb read_cb = nullptr;
    cpp_chardev_write_cb write_cb = nullptr;
    bool used = false;
};

MockCharDevSlot g_chardev_slots[k_max_mock_chardev_slots]{};
size_t g_chardev_slot_count = 0;

void refresh_registration_state();

bool names_equal(const MockCharDevSlot& slot, const char* name)
{
    size_t len = 0;
    while (len <= k_max_mock_name_len && name[len] != '\0')
    {
        ++len;
    }
    if (len != slot.name_len)
    {
        return false;
    }
    return std::memcmp(slot.name, name, len) == 0;
}

MockCharDevSlot* find_slot_by_name(const char* name)
{
    for (size_t idx = 0; idx < k_max_mock_chardev_slots; ++idx)
    {
        MockCharDevSlot& slot = g_chardev_slots[idx];
        if (!slot.used)
        {
            continue;
        }
        if (names_equal(slot, name))
        {
            return &slot;
        }
    }
    return nullptr;
}

MockCharDevSlot* find_slot_by_ctx(void* ctx)
{
    for (size_t idx = 0; idx < k_max_mock_chardev_slots; ++idx)
    {
        MockCharDevSlot& slot = g_chardev_slots[idx];
        if (!slot.used)
        {
            continue;
        }
        if (slot.ctx == ctx)
        {
            return &slot;
        }
    }
    return nullptr;
}
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
    int g_mock_chardev_registered_count = 0;
    int g_mock_chardev_reg_fail = 0;

    void cpp_mock_chardev_reset(void)
    {
        for (size_t idx = 0; idx < k_max_mock_chardev_slots; ++idx)
        {
            g_chardev_slots[idx] = MockCharDevSlot{};
        }
        g_chardev_slot_count = 0;
        refresh_registration_state();
    }

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
        (void)mode;
        if (name == nullptr || ctx == nullptr || read_cb == nullptr || write_cb == nullptr)
        {
            return -22; /* EINVAL */
        }
        if (g_mock_chardev_reg_fail != 0)
        {
            g_mock_chardev_reg_fail = 0;
            return -5; /* EIO */
        }
        if (find_slot_by_ctx(ctx) != nullptr || find_slot_by_name(name) != nullptr)
        {
            return -16; /* EBUSY */
        }

        size_t name_len = 0;
        while (name[name_len] != '\0')
        {
            ++name_len;
            if (name_len > k_max_mock_name_len)
            {
                return -22; /* EINVAL */
            }
        }

        for (size_t idx = 0; idx < k_max_mock_chardev_slots; ++idx)
        {
            MockCharDevSlot& slot = g_chardev_slots[idx];
            if (slot.used)
            {
                continue;
            }
            slot.used = true;
            slot.name_len = name_len;
            std::memcpy(slot.name, name, name_len);
            slot.name[name_len] = '\0';
            slot.ctx = ctx;
            slot.read_cb = read_cb;
            slot.write_cb = write_cb;
            ++g_chardev_slot_count;
            refresh_registration_state();
            return 0;
        }

        return -12; /* ENOMEM */
    }

    void cpp_userspace_chardev_unregister(void* ctx)
    {
        if (ctx == nullptr)
        {
            return;
        }

        for (size_t idx = 0; idx < k_max_mock_chardev_slots; ++idx)
        {
            MockCharDevSlot& slot = g_chardev_slots[idx];
            if (!slot.used)
            {
                continue;
            }
            if (slot.ctx == ctx)
            {
                slot = MockCharDevSlot{};
                if (g_chardev_slot_count > 0U)
                {
                    --g_chardev_slot_count;
                }
                break;
            }
        }

        refresh_registration_state();
    }

    cpp_ssize_t cpp_mock_chardev_simulate_read_named(const char* name, void* kbuf, size_t len,
                                                     std::int64_t* pos);
    cpp_ssize_t cpp_mock_chardev_simulate_write_named(const char* name, const void* kbuf,
                                                      size_t len, std::int64_t* pos);

    cpp_ssize_t cpp_mock_chardev_simulate_read(void* kbuf, size_t len, std::int64_t* pos)
    {
        return cpp_mock_chardev_simulate_read_named("cpp_lkm", kbuf, len, pos);
    }

    cpp_ssize_t cpp_mock_chardev_simulate_write(const void* kbuf, size_t len, std::int64_t* pos)
    {
        return cpp_mock_chardev_simulate_write_named("cpp_lkm", kbuf, len, pos);
    }

    cpp_ssize_t cpp_mock_chardev_simulate_read_named(const char* name, void* kbuf, size_t len,
                                                     std::int64_t* pos)
    {
        MockCharDevSlot* slot = find_slot_by_name(name);
        if (slot == nullptr || slot->read_cb == nullptr || slot->ctx == nullptr)
        {
            return -22;
        }
        return slot->read_cb(slot->ctx, kbuf, len, pos);
    }

    cpp_ssize_t cpp_mock_chardev_simulate_write_named(const char* name, const void* kbuf,
                                                      size_t len, std::int64_t* pos)
    {
        MockCharDevSlot* slot = find_slot_by_name(name);
        if (slot == nullptr || slot->write_cb == nullptr || slot->ctx == nullptr)
        {
            return -22;
        }
        return slot->write_cb(slot->ctx, kbuf, len, pos);
    }
}

namespace
{
void refresh_registration_state()
{
    g_mock_chardev_registered_count = static_cast<int>(g_chardev_slot_count);
    g_mock_chardev_registered = static_cast<int>(g_chardev_slot_count > 0U);
}
} // namespace
