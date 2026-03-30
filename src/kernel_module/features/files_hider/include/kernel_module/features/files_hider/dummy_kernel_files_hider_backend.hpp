#pragma once

#include "kernel_module/features/files_hider/i_kernel_files_hider_backend.hpp"

class DummyKernelFilesHiderBackend final : public IKernelFilesHiderBackend
{
  public:
    void hide_pattern(const char* pattern, size_t pattern_len) override;
    void unhide_pattern(const char* pattern, size_t pattern_len) override;
    void update_statistics() override;
};
