#pragma once

#include "kernel_module/core/chardev/ihandler.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

class KernelFilesHiderServer final : public IUserspaceDeviceHandler
{
  public:
    [[nodiscard]] cpp_ssize_t read_kernel(void* kbuf, size_t len, const std::int64_t* pos) override;
    [[nodiscard]] cpp_ssize_t write_kernel(const void* kbuf, size_t len,
                                           const std::int64_t* pos) override;

  private:
    static constexpr size_t k_max_patterns = 32;
    static constexpr size_t k_max_pattern_len = 127;
    static constexpr size_t k_max_response_len = 512;
    static constexpr size_t k_max_command_len = 256;

    struct HiddenPatternStats
    {
        std::array<char, k_max_pattern_len + 1U> pattern{};
        size_t pattern_len = 0;
        std::uint64_t matches = 0;
        std::uint64_t misses = 0;
        bool used = false;
    };

    void handle_command(const char* command, size_t command_len);
    void add_hidden_pattern(const char* pattern, size_t pattern_len);
    void remove_hidden_pattern(const char* pattern, size_t pattern_len);
    void list_hidden_patterns();
    void list_hidden_patterns_with_stats();
    void set_response(const char* content, size_t content_len);
    void set_response_literal(const char* content);

    [[nodiscard]] static size_t trim_command_len(const char* command, size_t command_len);
    [[nodiscard]] static bool pattern_equals(const HiddenPatternStats& item, const char* pattern,
                                             size_t pattern_len);
    [[nodiscard]] bool append_to_response(char character);
    [[nodiscard]] bool append_to_response(const char* text, size_t text_len);
    [[nodiscard]] bool append_u64_to_response(std::uint64_t value);
    void wrap_current_content_as_response();

    static void hide_pattern_hook(const char* pattern, size_t pattern_len);
    static void unhide_pattern_hook(const char* pattern, size_t pattern_len);
    static void update_statistics_hook();

    std::array<HiddenPatternStats, k_max_patterns> _hidden_patterns{};
    std::array<char, k_max_response_len> _response_buf{};
    size_t _response_len = 0;
};
