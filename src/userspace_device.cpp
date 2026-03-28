#include "my_module/userspace_device.hpp"

#include <cstring>
#include <string_view>

static constexpr std::string_view k_default_status{"cpp_lkm ok\n"};

static cpp_ssize_t userspace_read_impl(void* ctx, void* kbuf, size_t len, std::int64_t* pos)
{
    return static_cast<UserspaceDevice*>(ctx)->read_kernel(kbuf, len, pos);
}

static cpp_ssize_t userspace_write_impl(void* ctx, const void* kbuf, size_t len,
                                        std::int64_t* pos)
{
    return static_cast<UserspaceDevice*>(ctx)->write_kernel(kbuf, len, pos);
}

extern "C"
{
    cpp_ssize_t userspace_read_trampoline(void* ctx, void* kbuf, size_t len, std::int64_t* pos)
    {
        return userspace_read_impl(ctx, kbuf, len, pos);
    }

    cpp_ssize_t userspace_write_trampoline(void* ctx, const void* kbuf, size_t len,
                                           std::int64_t* pos)
    {
        return userspace_write_impl(ctx, kbuf, len, pos);
    }
}

cpp_ssize_t UserspaceDevice::read_kernel(void* kbuf, size_t len, const std::int64_t* pos)
{
    (void)pos;
    if (_write_len > 0)
    {
        const size_t copy_len = _write_len < len ? _write_len : len;
        memcpy(kbuf, _write_buf.data(), copy_len);
        return static_cast<cpp_ssize_t>(copy_len);
    }
    const size_t status_len = k_default_status.size();
    const size_t copy_len = status_len < len ? status_len : len;
    memcpy(kbuf, k_default_status.data(), copy_len);
    return static_cast<cpp_ssize_t>(copy_len);
}

cpp_ssize_t UserspaceDevice::write_kernel(const void* kbuf, size_t len, const std::int64_t* pos)
{
    (void)pos;
    if (len == 0)
    {
        return 0;
    }
    const size_t cap = k_buf_size - 1U;
    const size_t copy_len = len < cap ? len : cap;
    memcpy(_write_buf.data(), kbuf, copy_len);
    // copy_len < k_buf_size - 1 is guaranteed by cap; subscript is not a compile-time constant.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    _write_buf[copy_len] = '\0';
    _write_len = copy_len;
    return static_cast<cpp_ssize_t>(copy_len);
}

Result<void> UserspaceDevice::init()
{
    const int err = cpp_userspace_chardev_register(
        "cpp_lkm", 0666U, this, &userspace_read_trampoline, &userspace_write_trampoline);
    if (err != 0)
    {
        _registered = false;
        return std::unexpected(to_errno(MyError::CharDevRegFail));
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
