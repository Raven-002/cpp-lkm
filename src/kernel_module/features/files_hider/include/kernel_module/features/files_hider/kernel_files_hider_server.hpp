#pragma once

#include "kernel_module/core/chardev/ihandler.hpp"
#include "kernel_module/features/files_hider/dummy_kernel_files_hider_backend.hpp"
#include "kernel_module/features/files_hider/kernel_files_hider_manager.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

class KernelFilesHiderServer final : public IUserspaceDeviceHandler
{
  public:
    KernelFilesHiderServer();

    [[nodiscard]] cpp_ssize_t read_kernel(void* kbuf, size_t len, const std::int64_t* pos) override;
    [[nodiscard]] cpp_ssize_t write_kernel(const void* kbuf, size_t len,
                                           const std::int64_t* pos) override;

  private:
    static constexpr size_t k_max_response_len = 512;
    static constexpr size_t k_max_command_len = 256;
    static_assert(k_max_response_len >= 4U);

    void handle_command(std::string_view command);
    void list_hidden_patterns();
    void list_hidden_patterns_with_stats();
    void set_response(std::string_view content);

    [[nodiscard]] static std::string_view trim_command(std::string_view command);
    [[nodiscard]] bool append_to_response(char character);
    [[nodiscard]] bool append_to_response(std::string_view text);
    [[nodiscard]] bool append_u64_to_response(std::uint64_t value);
    void wrap_current_content_as_response();

    DummyKernelFilesHiderBackend _backend;
    KernelFilesHiderManager _manager;
    std::array<char, k_max_response_len> _response_buf{};
    size_t _response_len = 0;
};
