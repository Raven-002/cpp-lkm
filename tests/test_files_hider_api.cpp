// tests/test_files_hider_api.cpp
// Host tests for files_hider command parsing and response framing.
#include "kernel_module/features/files_hider/kernel_files_hider_server.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

static std::string_view write_then_read(KernelFilesHiderServer& server, std::string_view command,
                                        std::array<char, 512>& buffer)
{
    std::int64_t pos = 0;
    const cpp_ssize_t nwritten = server.write_kernel(command.data(), command.size(), &pos);
    assert(static_cast<size_t>(nwritten) == command.size());

    buffer.fill('\0');
    pos = 0;
    const cpp_ssize_t nread = server.read_kernel(buffer.data(), buffer.size(), &pos);
    assert(nread > 0);
    return std::string_view{buffer.data(), static_cast<size_t>(nread)};
}

static void test_add_response_framing()
{
    KernelFilesHiderServer server;
    std::array<char, 512> buffer{};
    const auto response = write_then_read(server, "+secret_.*", buffer);
    assert(response == "@{Added}@");
}

static void test_remove_missing_pattern()
{
    KernelFilesHiderServer server;
    std::array<char, 512> buffer{};
    const auto response = write_then_read(server, "-missing_pattern", buffer);
    assert(response == "@{Not Found}@");
}

static void test_list_patterns()
{
    KernelFilesHiderServer server;
    std::array<char, 512> buffer{};

    (void)write_then_read(server, "+foo_.*", buffer);
    (void)write_then_read(server, "+bar_.*", buffer);

    const auto response = write_then_read(server, "@list", buffer);
    assert(response == "@{foo_.*\nbar_.*}@");
}

static void test_stats_patterns()
{
    KernelFilesHiderServer server;
    std::array<char, 512> buffer{};

    (void)write_then_read(server, "+foo_.*", buffer);
    const auto response = write_then_read(server, "@stats", buffer);
    assert(response == "@{foo_.* matches=0 misses=0}@");
}

static void test_read_reaches_eof_on_same_fd()
{
    KernelFilesHiderServer server;
    std::array<char, 512> buffer{};

    std::int64_t write_pos = 0;
    const std::string_view command{"+single_read"};
    const cpp_ssize_t nwritten = server.write_kernel(command.data(), command.size(), &write_pos);
    assert(static_cast<size_t>(nwritten) == command.size());

    std::int64_t read_pos = 0;
    buffer.fill('\0');
    const cpp_ssize_t nread_first = server.read_kernel(buffer.data(), buffer.size(), &read_pos);
    assert(nread_first > 0);
    const auto response = std::string_view{buffer.data(), static_cast<size_t>(nread_first)};
    assert(response == "@{Added}@");

    buffer.fill('\0');
    const cpp_ssize_t nread_second = server.read_kernel(buffer.data(), buffer.size(), &read_pos);
    assert(nread_second == 0);
}

static void test_read_restarts_for_new_fd_position()
{
    KernelFilesHiderServer server;
    std::array<char, 512> buffer{};

    std::int64_t write_pos = 0;
    const std::string_view command{"+fd_reset"};
    const cpp_ssize_t nwritten = server.write_kernel(command.data(), command.size(), &write_pos);
    assert(static_cast<size_t>(nwritten) == command.size());

    std::int64_t first_fd_pos = 0;
    buffer.fill('\0');
    const cpp_ssize_t nread_first = server.read_kernel(buffer.data(), buffer.size(), &first_fd_pos);
    assert(nread_first > 0);

    buffer.fill('\0');
    const cpp_ssize_t nread_first_eof =
        server.read_kernel(buffer.data(), buffer.size(), &first_fd_pos);
    assert(nread_first_eof == 0);

    std::int64_t second_fd_pos = 0;
    buffer.fill('\0');
    const cpp_ssize_t nread_second_fd =
        server.read_kernel(buffer.data(), buffer.size(), &second_fd_pos);
    assert(nread_second_fd == nread_first);
    const auto response = std::string_view{buffer.data(), static_cast<size_t>(nread_second_fd)};
    assert(response == "@{Added}@");
}

extern "C" int main()
{
    test_add_response_framing();
    test_remove_missing_pattern();
    test_list_patterns();
    test_stats_patterns();
    test_read_reaches_eof_on_same_fd();
    test_read_restarts_for_new_fd_position();
    return 0;
}
