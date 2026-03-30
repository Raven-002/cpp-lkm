#pragma once

#include "kernel_api/kernel_api.h"

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
