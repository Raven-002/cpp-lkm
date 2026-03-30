// Host-test helpers to exercise the char-device bridge without a real /dev node.
#pragma once

#include "kernel_api/kernel_api.h"

#include <cstddef>
#include <cstdint>

#ifdef __cplusplus
extern "C"
{
#endif

    cpp_ssize_t cpp_mock_chardev_simulate_read(void* kbuf, size_t len, std::int64_t* pos);
    cpp_ssize_t cpp_mock_chardev_simulate_write(const void* kbuf, size_t len, std::int64_t* pos);
    cpp_ssize_t cpp_mock_chardev_simulate_read_named(const char* name, void* kbuf, size_t len,
                                                     std::int64_t* pos);
    cpp_ssize_t cpp_mock_chardev_simulate_write_named(const char* name, const void* kbuf, size_t len,
                                                      std::int64_t* pos);

#ifdef __cplusplus
}
#endif
