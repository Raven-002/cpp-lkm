#pragma once

#include "cpp_lkm/runtime/kernel_api.h"
#include "errors.hpp" // IWYU pragma: keep

#include <cstddef>
#include <cstdint>

class IUserspaceDeviceHandler
{
  public:
    IUserspaceDeviceHandler() = default;
    virtual ~IUserspaceDeviceHandler() = default;
    IUserspaceDeviceHandler(const IUserspaceDeviceHandler&) = default;
    IUserspaceDeviceHandler& operator=(const IUserspaceDeviceHandler&) = default;
    IUserspaceDeviceHandler(IUserspaceDeviceHandler&&) = default;
    IUserspaceDeviceHandler& operator=(IUserspaceDeviceHandler&&) = default;

    [[nodiscard]] virtual cpp_ssize_t read_kernel(void* kbuf, size_t len,
                                                  const std::int64_t* pos) = 0;
    [[nodiscard]] virtual cpp_ssize_t write_kernel(const void* kbuf, size_t len,
                                                   const std::int64_t* pos) = 0;
};

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
