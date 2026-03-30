#pragma once

#include "kernel_module/core/chardev/ihandler.hpp"
#include "kernel_module/errors.hpp" // IWYU pragma: keep

// Userspace-facing character device: read/write via the runtime bridge (miscdevice).
class UserspaceDevice
{
  public:
    explicit UserspaceDevice(IUserspaceDeviceHandler& handler);

    UserspaceDevice(const UserspaceDevice&) = delete;
    UserspaceDevice& operator=(const UserspaceDevice&) = delete;
    UserspaceDevice(UserspaceDevice&&) = delete;
    UserspaceDevice& operator=(UserspaceDevice&&) = delete;

    [[nodiscard]] Result<void> init();
    ~UserspaceDevice();

    // Bridge callbacks (C linkage); not for general use.
    [[nodiscard]] cpp_ssize_t read_kernel(void* kbuf, size_t len, const std::int64_t* pos);
    [[nodiscard]] cpp_ssize_t write_kernel(const void* kbuf, size_t len, const std::int64_t* pos);

  private:
    IUserspaceDeviceHandler* _handler = nullptr;
    bool _registered = false;
};
