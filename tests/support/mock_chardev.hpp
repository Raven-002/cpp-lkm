// Host-test helpers to exercise the char-device bridge without a real /dev node.
#pragma once

#include "cpp_lkm/runtime/kernel_api.h"

#include <cstdint>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    cpp_ssize_t cpp_mock_chardev_simulate_read(void* kbuf, size_t len, std::int64_t* pos);
    cpp_ssize_t cpp_mock_chardev_simulate_write(const void* kbuf, size_t len, std::int64_t* pos);

#ifdef __cplusplus
}
#endif
