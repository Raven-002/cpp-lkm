#include "kernel_module/features/files_hider/kernel_files_hider_manager.hpp"

#include <cstring>
#include <limits>

KernelFilesHiderManager::KernelFilesHiderManager(IKernelFilesHiderBackend& backend)
    : _backend(&backend)
{
}

bool KernelFilesHiderManager::add_hidden_pattern(std::string_view pattern)
{
    const std::string_view stored_pattern = canonical_pattern(pattern);
    for (const HiddenPatternStats& item : _hidden_patterns)
    {
        if (item.used && pattern_equals(item, stored_pattern))
        {
            _backend->hide_pattern(stored_pattern, *this);
            return true;
        }
    }

    for (HiddenPatternStats& item : _hidden_patterns)
    {
        if (!item.used)
        {
            set_hidden_pattern(item, stored_pattern);
            _backend->hide_pattern(stored_pattern, *this);
            return true;
        }
    }

    // Preserve existing compatibility behavior: report Added even when no slot is available.
    _backend->hide_pattern(stored_pattern, *this);
    return true;
}

bool KernelFilesHiderManager::remove_hidden_pattern(std::string_view pattern)
{
    const std::string_view stored_pattern = canonical_pattern(pattern);
    for (HiddenPatternStats& item : _hidden_patterns)
    {
        if (item.used && pattern_equals(item, stored_pattern))
        {
            _backend->unhide_pattern(stored_pattern);
            reset_hidden_pattern(item);
            return true;
        }
    }

    return false;
}

void KernelFilesHiderManager::record_hide_applied(std::string_view pattern)
{
    HiddenPatternStats* item = find_hidden_pattern(pattern);
    if (item == nullptr)
    {
        return;
    }
    increment_counter(item->matches);
}

void KernelFilesHiderManager::record_hide_skipped(std::string_view pattern)
{
    HiddenPatternStats* item = find_hidden_pattern(pattern);
    if (item == nullptr)
    {
        return;
    }
    increment_counter(item->misses);
}

const std::array<KernelFilesHiderManager::HiddenPatternStats,
                 KernelFilesHiderManager::k_max_patterns>&
KernelFilesHiderManager::hidden_patterns() const
{
    return _hidden_patterns;
}

size_t KernelFilesHiderManager::bounded_pattern_len(const HiddenPatternStats& item)
{
    return item.pattern_len < k_max_pattern_len ? item.pattern_len : k_max_pattern_len;
}

std::string_view KernelFilesHiderManager::stored_pattern_view(const HiddenPatternStats& item)
{
    return {item.pattern.data(), bounded_pattern_len(item)};
}

std::string_view KernelFilesHiderManager::canonical_pattern(std::string_view pattern)
{
    if (pattern.size() > k_max_pattern_len)
    {
        pattern.remove_suffix(pattern.size() - k_max_pattern_len);
    }
    return pattern;
}

bool KernelFilesHiderManager::pattern_equals(const HiddenPatternStats& item,
                                             std::string_view pattern)
{
    return stored_pattern_view(item) == pattern;
}

KernelFilesHiderManager::HiddenPatternStats*
KernelFilesHiderManager::find_hidden_pattern(std::string_view pattern)
{
    const std::string_view stored_pattern = canonical_pattern(pattern);
    for (HiddenPatternStats& item : _hidden_patterns)
    {
        if (item.used && pattern_equals(item, stored_pattern))
        {
            return &item;
        }
    }
    return nullptr;
}

void KernelFilesHiderManager::increment_counter(std::uint64_t& counter)
{
    if (counter < std::numeric_limits<std::uint64_t>::max())
    {
        ++counter;
    }
}

void KernelFilesHiderManager::reset_hidden_pattern(HiddenPatternStats& item)
{
    std::memset(item.pattern.data(), 0, item.pattern.size());
    item.pattern_len = 0;
    item.matches = 0;
    item.misses = 0;
    item.used = false;
}

void KernelFilesHiderManager::set_hidden_pattern(HiddenPatternStats& item, std::string_view pattern)
{
    const std::string_view bounded = canonical_pattern(pattern);
    reset_hidden_pattern(item);
    std::memcpy(item.pattern.data(), bounded.data(), bounded.size());
    item.pattern_len = bounded.size();
    item.used = true;
}
