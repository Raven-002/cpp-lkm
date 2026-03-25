#include "cpp_lkm/module/userspace_device.hpp"

#include <string.h>

namespace
{
constexpr const char k_default_status[] = "cpp_lkm ok\n";
} // namespace

extern "C"
{
    static cpp_ssize_t userspace_read_trampoline(void* ctx, void* kbuf, size_t len,
                                                 std::int64_t* pos)
    {
        return static_cast<UserspaceDevice*>(ctx)->read_kernel(kbuf, len, pos);
    }

    static cpp_ssize_t userspace_write_trampoline(void* ctx, const void* kbuf, size_t len,
                                                  std::int64_t* pos)
    {
        return static_cast<UserspaceDevice*>(ctx)->write_kernel(kbuf, len, pos);
    }
} // extern "C"

cpp_ssize_t UserspaceDevice::read_kernel(void* kbuf, size_t len, std::int64_t* pos)
{
    (void)pos;
    if (_write_len > 0)
    {
        const size_t n = _write_len < len ? _write_len : len;
        memcpy(kbuf, _write_buf, n);
        return static_cast<cpp_ssize_t>(n);
    }
    const size_t status_len = sizeof(k_default_status) - 1U;
    const size_t n = status_len < len ? status_len : len;
    memcpy(kbuf, k_default_status, n);
    return static_cast<cpp_ssize_t>(n);
}

cpp_ssize_t UserspaceDevice::write_kernel(const void* kbuf, size_t len, std::int64_t* pos)
{
    (void)pos;
    if (len == 0)
        return 0;
    const size_t cap = k_buf_size - 1U;
    const size_t n = len < cap ? len : cap;
    memcpy(_write_buf, kbuf, n);
    _write_buf[n] = '\0';
    _write_len = n;
    return static_cast<cpp_ssize_t>(n);
}

Result<void> UserspaceDevice::init()
{
    const int err = cpp_userspace_chardev_register(
        "cpp_lkm", 0666U, this, &userspace_read_trampoline, &userspace_write_trampoline);
    if (err != 0)
    {
        _registered = false;
        return std::unexpected(ErrorCode::CharDevRegFail);
    }
    _registered = true;
    return {};
}

UserspaceDevice::~UserspaceDevice()
{
    if (_registered)
    {
        cpp_userspace_chardev_unregister();
        _registered = false;
    }
}
