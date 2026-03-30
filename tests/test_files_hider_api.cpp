// tests/test_files_hider_api.cpp
// Host tests for files_hider command parsing and response framing.
#include "kernel_module/features/files_hider/kernel_files_hider_server.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace
{
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
} // namespace

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

extern "C" int main()
{
    test_add_response_framing();
    test_remove_missing_pattern();
    test_list_patterns();
    test_stats_patterns();
    return 0;
}
