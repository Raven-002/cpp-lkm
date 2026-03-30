#pragma once

#include "kernel_module/features/files_hider/i_kernel_files_hider_backend.hpp"

class DummyKernelFilesHiderBackend final : public IKernelFilesHiderBackend
{
  public:
    void hide_pattern(std::string_view pattern, IKernelFilesHiderStatsSink& stats_sink) override;
    void unhide_pattern(std::string_view pattern) override;

  private:
    IKernelFilesHiderStatsSink* _stats_sink = nullptr;
};
