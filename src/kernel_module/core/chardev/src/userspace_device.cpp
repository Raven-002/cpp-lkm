#include "kernel_module/core/chardev/userspace_device.hpp"

static cpp_ssize_t userspace_read_impl(void* ctx, void* kbuf, size_t len, std::int64_t* pos)
{
    return static_cast<UserspaceDevice*>(ctx)->read_kernel(kbuf, len, pos);
}

static cpp_ssize_t userspace_write_impl(void* ctx, const void* kbuf, size_t len, std::int64_t* pos)
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

UserspaceDevice::UserspaceDevice(IUserspaceDeviceHandler& handler) : _handler(&handler) {}

cpp_ssize_t UserspaceDevice::read_kernel(void* kbuf, size_t len, const std::int64_t* pos)
{
    return _handler->read_kernel(kbuf, len, pos);
}

cpp_ssize_t UserspaceDevice::write_kernel(const void* kbuf, size_t len, const std::int64_t* pos)
{
    return _handler->write_kernel(kbuf, len, pos);
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
