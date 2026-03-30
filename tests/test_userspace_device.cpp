// tests/test_userspace_device.cpp
// Host tests for UserspaceDevice registration and mock read/write path.
#include "cpp_lkm/runtime/module_entry.h"
#include "kernel_module/errors.hpp"
#include "tests/support/mock_chardev.hpp"
#include "tests/support/mock_globals.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>
#include <utility>

static void test_chardev_registered_on_successful_init()
{
    reset_mock_state();
    assert(cpp_module_init() == 0);
    assert(g_mock_chardev_registered == 1);
    assert(g_mock_chardev_registered_count == 2);
    cpp_module_exit();
    assert(g_mock_chardev_registered == 0);
    assert(g_mock_chardev_registered_count == 0);
}

static void test_chardev_reg_fail_unwinds_module()
{
    reset_mock_state();
    g_mock_chardev_reg_fail = 1;
    assert(cpp_module_init() == to_errno(MyError::CharDevRegFail));
    assert(g_mock_chardev_registered == 0);
    cpp_module_exit();
    assert(g_mock_cpp_destructed_count == 1);
}

static void test_mock_read_default_status()
{
    reset_mock_state();
    assert(cpp_module_init() == 0);

    std::array<char, 64> buf{};
    std::int64_t pos = 0;
    const cpp_ssize_t nread = cpp_mock_chardev_simulate_read(buf.data(), buf.size(), &pos);
    assert(nread > 0);
    assert(strncmp(buf.data(), "cpp_lkm ok\n", static_cast<size_t>(nread)) == 0);

    cpp_module_exit();
}

static void test_mock_write_then_read_echo()
{
    reset_mock_state();
    assert(cpp_module_init() == 0);

    constexpr std::string_view msg{"hello"};
    constexpr std::size_t msg_len = msg.size();
    std::int64_t pos = 0;
    const cpp_ssize_t nwritten = cpp_mock_chardev_simulate_write(msg.data(), msg_len, &pos);
    assert(std::cmp_equal(nwritten, msg_len));

    std::array<char, 64> buf{};
    pos = 0;
    const cpp_ssize_t nread = cpp_mock_chardev_simulate_read(buf.data(), buf.size(), &pos);
    assert(std::cmp_equal(nread, msg_len));
    assert(memcmp(buf.data(), msg.data(), static_cast<std::size_t>(nread)) == 0);

    cpp_module_exit();
}

static void test_mock_write_truncates_to_device_capacity()
{
    reset_mock_state();
    assert(cpp_module_init() == 0);

    std::array<char, 300> msg{};
    msg.fill('x');
    std::int64_t pos = 0;
    const cpp_ssize_t nwritten = cpp_mock_chardev_simulate_write(msg.data(), msg.size(), &pos);
    assert(nwritten == 255);

    std::array<char, 300> buf{};
    pos = 0;
    const cpp_ssize_t nread = cpp_mock_chardev_simulate_read(buf.data(), buf.size(), &pos);
    assert(nread == 255);
    std::array<char, 255> expected{};
    expected.fill('x');
    assert(memcmp(buf.data(), expected.data(), expected.size()) == 0);

    cpp_module_exit();
}

static void test_multi_device_coexistence()
{
    reset_mock_state();
    assert(cpp_module_init() == 0);
    assert(g_mock_chardev_registered_count == 2);

    constexpr std::string_view hide_cmd{"+secret_.*"};
    std::int64_t pos = 0;
    const cpp_ssize_t hide_written = cpp_mock_chardev_simulate_write_named(
        "cpp_lkm_files_hider", hide_cmd.data(), hide_cmd.size(), &pos);
    assert(std::cmp_equal(hide_written, hide_cmd.size()));

    std::array<char, 64> hide_reply{};
    pos = 0;
    const cpp_ssize_t hide_reply_n = cpp_mock_chardev_simulate_read_named(
        "cpp_lkm_files_hider", hide_reply.data(), hide_reply.size(), &pos);
    assert(hide_reply_n > 0);
    assert(strncmp(hide_reply.data(), "@{Added}@", static_cast<size_t>(hide_reply_n)) == 0);

    constexpr std::string_view echo_msg{"hello"};
    pos = 0;
    const cpp_ssize_t echo_written =
        cpp_mock_chardev_simulate_write(echo_msg.data(), echo_msg.size(), &pos);
    assert(std::cmp_equal(echo_written, echo_msg.size()));

    std::array<char, 64> echo_buf{};
    pos = 0;
    const cpp_ssize_t echo_read =
        cpp_mock_chardev_simulate_read(echo_buf.data(), echo_buf.size(), &pos);
    assert(std::cmp_equal(echo_read, echo_msg.size()));
    assert(memcmp(echo_buf.data(), echo_msg.data(), static_cast<size_t>(echo_read)) == 0);

    cpp_module_exit();
    assert(g_mock_chardev_registered_count == 0);
}

extern "C" int main()
{
    test_chardev_registered_on_successful_init();
    test_chardev_reg_fail_unwinds_module();
    test_mock_read_default_status();
    test_mock_write_then_read_echo();
    test_mock_write_truncates_to_device_capacity();
    test_multi_device_coexistence();
    printf("All userspace device tests passed!\n");
    return 0;
}
