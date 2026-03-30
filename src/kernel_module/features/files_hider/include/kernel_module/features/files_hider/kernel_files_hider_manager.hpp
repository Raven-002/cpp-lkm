#pragma once

#include "kernel_module/features/files_hider/i_kernel_files_hider_backend.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

class KernelFilesHiderManager final
{
  public:
    static constexpr size_t k_max_patterns = 32;
    static constexpr size_t k_max_pattern_len = 127;

    struct HiddenPatternStats
    {
        std::array<char, k_max_pattern_len + 1U> pattern{};
        size_t pattern_len = 0;
        std::uint64_t matches = 0;
        std::uint64_t misses = 0;
        bool used = false;
    };

    explicit KernelFilesHiderManager(IKernelFilesHiderBackend& backend);

    [[nodiscard]] bool add_hidden_pattern(std::string_view pattern);
    [[nodiscard]] bool remove_hidden_pattern(std::string_view pattern);
    void update_statistics();

    [[nodiscard]] const std::array<HiddenPatternStats, k_max_patterns>& hidden_patterns() const;
    [[nodiscard]] static size_t bounded_pattern_len(const HiddenPatternStats& item);
    [[nodiscard]] static std::string_view stored_pattern_view(const HiddenPatternStats& item);

  private:
    [[nodiscard]] static std::string_view canonical_pattern(std::string_view pattern);
    [[nodiscard]] static bool pattern_equals(const HiddenPatternStats& item,
                                             std::string_view pattern);
    static void reset_hidden_pattern(HiddenPatternStats& item);
    static void set_hidden_pattern(HiddenPatternStats& item, std::string_view pattern);

    IKernelFilesHiderBackend* _backend = nullptr;
    std::array<HiddenPatternStats, k_max_patterns> _hidden_patterns{};
};
