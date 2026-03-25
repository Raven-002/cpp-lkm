// tests/test_userspace_device.cpp
// Host tests for UserspaceDevice registration and mock read/write path.
#include "cpp_lkm/common/error.hpp"
#include "cpp_lkm/runtime/module_entry.h"
#include "tests/support/mock_chardev.hpp"
#include "tests/support/mock_globals.hpp"

#include <assert.h>
#include <cstddef>
#include <cstdint>
#include <stdio.h>
#include <string.h>
#include <utility>

void test_chardev_registered_on_successful_init()
{
    reset_mock_state();
    assert(cpp_module_init() == 0);
    assert(__mock_chardev_registered == 1);
    cpp_module_exit();
    assert(__mock_chardev_registered == 0);
}

void test_chardev_reg_fail_unwinds_module()
{
    reset_mock_state();
    __mock_chardev_reg_fail = 1;
    assert(cpp_module_init() == to_errno(ErrorCode::CharDevRegFail));
    assert(__mock_chardev_registered == 0);
    cpp_module_exit();
    assert(__mock_cpp_destructed_count == 1);
}

void test_mock_read_default_status()
{
    reset_mock_state();
    assert(cpp_module_init() == 0);

    char buf[64]{};
    std::int64_t pos = 0;
    cpp_ssize_t n = cpp_mock_chardev_simulate_read(buf, sizeof(buf), &pos);
    assert(n > 0);
    assert(strncmp(buf, "cpp_lkm ok\n", (size_t)n) == 0);

    cpp_module_exit();
}

void test_mock_write_then_read_echo()
{
    reset_mock_state();
    assert(cpp_module_init() == 0);

    const char msg[] = "hello";
    constexpr std::size_t msg_len = sizeof(msg) - 1U;
    std::int64_t pos = 0;
    cpp_ssize_t wn = cpp_mock_chardev_simulate_write(msg, msg_len, &pos);
    assert(std::cmp_equal(wn, msg_len));

    char buf[64]{};
    pos = 0;
    cpp_ssize_t rn = cpp_mock_chardev_simulate_read(buf, sizeof(buf), &pos);
    assert(std::cmp_equal(rn, msg_len));
    assert(memcmp(buf, msg, static_cast<std::size_t>(rn)) == 0);

    cpp_module_exit();
}

extern "C" int main()
{
    test_chardev_registered_on_successful_init();
    test_chardev_reg_fail_unwinds_module();
    test_mock_read_default_status();
    test_mock_write_then_read_echo();
    printf("All userspace device tests passed!\n");
    return 0;
}
