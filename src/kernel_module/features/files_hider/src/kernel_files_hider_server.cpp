#include "kernel_module/features/files_hider/kernel_files_hider_server.hpp"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <string_view>

static constexpr std::string_view k_list_command{"@list"};
static constexpr std::string_view k_stats_command{"@stats"};
static constexpr std::string_view k_matches_label{" matches="};
static constexpr std::string_view k_misses_label{" misses="};
static constexpr size_t k_u64_digits_required = 20U;
static constexpr size_t k_u64_digits_max = 20U;
static constexpr std::uint64_t k_decimal_base = 10U;
static_assert(k_u64_digits_max >= k_u64_digits_required);

[[nodiscard]] static std::string_view view_from_offset(std::string_view text, size_t offset)
{
    if (offset >= text.size())
    {
        return {};
    }
    text.remove_prefix(offset);
    return text;
}

[[nodiscard]] static std::string_view view_with_max_len(std::string_view text, size_t max_len)
{
    if (text.size() > max_len)
    {
        text.remove_suffix(text.size() - max_len);
    }
    return text;
}

KernelFilesHiderServer::KernelFilesHiderServer() : _backend(), _manager(_backend) {}

cpp_ssize_t KernelFilesHiderServer::read_kernel(void* kbuf, size_t len, const std::int64_t* pos)
{
    if (len == 0)
    {
        return 0;
    }

    if (kbuf == nullptr || pos == nullptr)
    {
        return 0;
    }

    const std::int64_t raw_offset = *pos;
    if (raw_offset < 0)
    {
        return 0;
    }

    const auto offset = static_cast<size_t>(raw_offset);
    if (offset >= _response_len)
    {
        return 0;
    }

    const size_t remaining = _response_len - offset;
    const size_t copy_len = remaining < len ? remaining : len;
    const void* src = std::next(_response_buf.data(), static_cast<std::ptrdiff_t>(offset));
    std::memcpy(kbuf, src, copy_len);

    // The bridge passes a mutable file offset, but this interface exposes it as const.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    auto* mutable_pos = const_cast<std::int64_t*>(pos);
    *mutable_pos += static_cast<std::int64_t>(copy_len);
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

    if (kbuf == nullptr)
    {
        set_response("Invalid Command");
        return 0;
    }
    std::array<char, k_max_command_len + 1U> command_buf{};
    const size_t copy_len = len < k_max_command_len ? len : k_max_command_len;
    std::memcpy(command_buf.data(), kbuf, copy_len);
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
        (void)_manager.add_hidden_pattern(view_from_offset(command, 1U));
        set_response("Added");
        return;
    }
    if (command.front() == '-')
    {
        if (command.size() == 1U)
        {
            set_response("Invalid Command");
            return;
        }
        const bool removed = _manager.remove_hidden_pattern(view_from_offset(command, 1U));
        set_response(removed ? "Removed" : "Not Found");
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

void KernelFilesHiderServer::list_hidden_patterns()
{
    _response_len = 0;
    size_t listed = 0;
    for (const KernelFilesHiderManager::HiddenPatternStats& item : _manager.hidden_patterns())
    {
        if (!item.used)
        {
            continue;
        }
        if (listed > 0U)
        {
            (void)append_to_response('\n');
        }
        (void)append_to_response(KernelFilesHiderManager::stored_pattern_view(item));
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
    _manager.update_statistics();
    _response_len = 0;
    size_t listed = 0;
    for (const KernelFilesHiderManager::HiddenPatternStats& item : _manager.hidden_patterns())
    {
        if (!item.used)
        {
            continue;
        }
        if (listed > 0U)
        {
            (void)append_to_response('\n');
        }
        (void)append_to_response(KernelFilesHiderManager::stored_pattern_view(item));
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

bool KernelFilesHiderServer::append_to_response(char character)
{
    if (_response_len > _response_buf.size())
    {
        return false;
    }
    if (_response_len >= _response_buf.size())
    {
        return false;
    }
    *std::next(_response_buf.data(), static_cast<std::ptrdiff_t>(_response_len)) = character;
    ++_response_len;
    return true;
}

bool KernelFilesHiderServer::append_to_response(std::string_view text)
{
    if (_response_len > _response_buf.size())
    {
        return false;
    }
    if (_response_len + text.size() > _response_buf.size())
    {
        text = view_with_max_len(text, _response_buf.size() - _response_len);
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
        *digits.begin() = '0';
        digits_len = 1U;
    }
    else
    {
        while (value > 0U && digits_len < digits.size())
        {
            *std::next(digits.data(), static_cast<std::ptrdiff_t>(digits_len)) =
                static_cast<char>('0' + (value % k_decimal_base));
            value /= k_decimal_base;
            ++digits_len;
        }
    }

    while (digits_len > 0U)
    {
        --digits_len;
        if (!append_to_response(*std::next(digits.data(), static_cast<std::ptrdiff_t>(digits_len))))
        {
            return false;
        }
    }
    return true;
}

void KernelFilesHiderServer::wrap_current_content_as_response()
{
    std::array<char, k_max_response_len> content{};
    const size_t content_len =
        _response_len < _response_buf.size() ? _response_len : _response_buf.size();
    if (content_len > 0U)
    {
        std::memcpy(content.data(), _response_buf.data(), content_len);
    }
    set_response({content.data(), content_len});
}
