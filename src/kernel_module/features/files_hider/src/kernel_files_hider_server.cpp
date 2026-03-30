#include "kernel_module/features/files_hider/kernel_files_hider_server.hpp"

#include "kernel_api/kernel_api.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <limits>
#include <string_view>

namespace
{
constexpr std::string_view k_list_command{"@list"};
constexpr std::string_view k_stats_command{"@stats"};
constexpr std::string_view k_matches_label{" matches="};
constexpr std::string_view k_misses_label{" misses="};
constexpr size_t k_u64_digits_max = 20U;
constexpr std::uint64_t k_decimal_base = 10U;

} // namespace

cpp_ssize_t KernelFilesHiderServer::read_kernel(void* kbuf, size_t len, const std::int64_t* pos)
{
    (void)pos;
    if (len == 0)
    {
        return 0;
    }

    const size_t copy_len = _response_len < len ? _response_len : len;
    memcpy(kbuf, _response_buf.data(), copy_len);
    return static_cast<cpp_ssize_t>(copy_len);
}

cpp_ssize_t KernelFilesHiderServer::write_kernel(const void* kbuf, size_t len,
                                                 const std::int64_t* pos)
{
    (void)pos;
    if (_response_len == 0U)
    {
        set_response("Ready");
    }
    if (len == 0)
    {
        set_response("Invalid Command");
        return 0;
    }

    std::array<char, k_max_command_len + 1U> command_buf{};
    const size_t copy_len = len < k_max_command_len ? len : k_max_command_len;
    memcpy(command_buf.data(), kbuf, copy_len);
    const std::string_view command = trim_command({command_buf.data(), copy_len});
    handle_command(command);
    return static_cast<cpp_ssize_t>(len);
}

void KernelFilesHiderServer::handle_command(std::string_view command)
{
    if (command.empty())
    {
        set_response("Invalid Command");
        return;
    }
    if (command.front() == '+')
    {
        if (command.size() == 1U)
        {
            set_response("Invalid Command");
            return;
        }
        add_hidden_pattern(command.substr(1U));
        return;
    }
    if (command.front() == '-')
    {
        if (command.size() == 1U)
        {
            set_response("Invalid Command");
            return;
        }
        remove_hidden_pattern(command.substr(1U));
        return;
    }
    if (command == k_list_command)
    {
        list_hidden_patterns();
        return;
    }
    if (command == k_stats_command)
    {
        list_hidden_patterns_with_stats();
        return;
    }

    set_response("Invalid Command");
}

void KernelFilesHiderServer::add_hidden_pattern(std::string_view pattern)
{
    const std::string_view stored_pattern = pattern.substr(0U, k_max_pattern_len);
    for (const HiddenPatternStats& item : _hidden_patterns)
    {
        if (item.used && pattern_equals(item, stored_pattern))
        {
            hide_pattern_hook(stored_pattern.data(), stored_pattern.size());
            set_response("Added");
            return;
        }
    }

    for (HiddenPatternStats& item : _hidden_patterns)
    {
        if (!item.used)
        {
            item.used = true;
            item.pattern_len = stored_pattern.size();
            std::memset(item.pattern.data(), 0, item.pattern.size());
            std::memcpy(item.pattern.data(), stored_pattern.data(), stored_pattern.size());
            item.matches = 0;
            item.misses = 0;
            hide_pattern_hook(stored_pattern.data(), stored_pattern.size());
            set_response("Added");
            return;
        }
    }

    hide_pattern_hook(stored_pattern.data(), stored_pattern.size());
    set_response("Added");
}

void KernelFilesHiderServer::remove_hidden_pattern(std::string_view pattern)
{
    for (HiddenPatternStats& item : _hidden_patterns)
    {
        if (item.used && pattern_equals(item, pattern))
        {
            unhide_pattern_hook(pattern.data(), pattern.size());
            item.used = false;
            item.pattern_len = 0;
            item.matches = 0;
            item.misses = 0;
            std::memset(item.pattern.data(), 0, item.pattern.size());
            set_response("Removed");
            return;
        }
    }

    set_response("Not Found");
}

void KernelFilesHiderServer::list_hidden_patterns()
{
    _response_len = 0;
    size_t listed = 0;
    for (const HiddenPatternStats& item : _hidden_patterns)
    {
        if (!item.used)
        {
            continue;
        }
        if (listed > 0U)
        {
            (void)append_to_response('\n');
        }
        (void)append_to_response({item.pattern.data(), item.pattern_len});
        ++listed;
    }

    if (listed == 0U)
    {
        set_response("Empty");
        return;
    }
    wrap_current_content_as_response();
}

void KernelFilesHiderServer::list_hidden_patterns_with_stats()
{
    update_statistics_hook();
    _response_len = 0;
    size_t listed = 0;
    for (const HiddenPatternStats& item : _hidden_patterns)
    {
        if (!item.used)
        {
            continue;
        }
        if (listed > 0U)
        {
            (void)append_to_response('\n');
        }
        (void)append_to_response({item.pattern.data(), item.pattern_len});
        (void)append_to_response(k_matches_label);
        (void)append_u64_to_response(item.matches);
        (void)append_to_response(k_misses_label);
        (void)append_u64_to_response(item.misses);
        ++listed;
    }

    if (listed == 0U)
    {
        set_response("Empty");
        return;
    }
    wrap_current_content_as_response();
}

void KernelFilesHiderServer::set_response(std::string_view content)
{
    _response_len = 0;
    (void)append_to_response("@{");
    (void)append_to_response(content);
    (void)append_to_response("}@");
}

std::string_view KernelFilesHiderServer::trim_command(std::string_view command)
{
    while (!command.empty())
    {
        const char last = command.back();
        if (last == ' ' || last == '\t' || last == '\n' || last == '\r')
        {
            command.remove_suffix(1U);
            continue;
        }
        break;
    }
    return command;
}

bool KernelFilesHiderServer::pattern_equals(const HiddenPatternStats& item,
                                            std::string_view pattern)
{
    return std::string_view{item.pattern.data(), item.pattern_len} == pattern;
}

bool KernelFilesHiderServer::append_to_response(char character)
{
    if (_response_len >= _response_buf.size())
    {
        return false;
    }
    _response_buf.at(_response_len) = character;
    ++_response_len;
    return true;
}

bool KernelFilesHiderServer::append_to_response(std::string_view text)
{
    if (_response_len + text.size() > _response_buf.size())
    {
        text = text.substr(0U, _response_buf.size() - _response_len);
    }
    if (text.empty())
    {
        return false;
    }
    auto* response_it = std::next(_response_buf.data(), static_cast<std::ptrdiff_t>(_response_len));
    std::copy_n(text.data(), text.size(), response_it);
    _response_len += text.size();
    return true;
}

bool KernelFilesHiderServer::append_u64_to_response(std::uint64_t value)
{
    std::array<char, k_u64_digits_max> digits{};
    size_t digits_len = 0;
    if (value == 0U)
    {
        digits.at(0U) = '0';
        digits_len = 1U;
    }
    else
    {
        while (value > 0U && digits_len < digits.size())
        {
            digits.at(digits_len) = static_cast<char>('0' + (value % k_decimal_base));
            value /= k_decimal_base;
            ++digits_len;
        }
    }

    while (digits_len > 0U)
    {
        --digits_len;
        if (!append_to_response(digits.at(digits_len)))
        {
            return false;
        }
    }
    return true;
}

void KernelFilesHiderServer::wrap_current_content_as_response()
{
    std::array<char, k_max_response_len> content{};
    const size_t content_len = _response_len;
    if (content_len > 0U)
    {
        std::memcpy(content.data(), _response_buf.data(), content_len);
    }
    set_response({content.data(), content_len});
}

void KernelFilesHiderServer::hide_pattern_hook(const char* pattern, size_t pattern_len)
{
    const auto max_log_len = static_cast<size_t>(std::numeric_limits<int>::max());
    const int log_len = static_cast<int>(pattern_len < max_log_len ? pattern_len : max_log_len);
    cpp_printk(CPP_KERN_INFO "[CPP] FilesHider hide_pattern_hook called for: %.*s\n", log_len,
               pattern);
}

void KernelFilesHiderServer::unhide_pattern_hook(const char* pattern, size_t pattern_len)
{
    const auto max_log_len = static_cast<size_t>(std::numeric_limits<int>::max());
    const int log_len = static_cast<int>(pattern_len < max_log_len ? pattern_len : max_log_len);
    cpp_printk(CPP_KERN_INFO "[CPP] FilesHider unhide_pattern_hook called for: %.*s\n", log_len,
               pattern);
}

void KernelFilesHiderServer::update_statistics_hook()
{
    cpp_printk(CPP_KERN_INFO "[CPP] FilesHider update_statistics_hook called\n");
}
