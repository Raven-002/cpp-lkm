#pragma once

#include <cstddef>

class IKernelFilesHiderBackend
{
  public:
    IKernelFilesHiderBackend() = default;
    IKernelFilesHiderBackend(const IKernelFilesHiderBackend&) = default;
    IKernelFilesHiderBackend& operator=(const IKernelFilesHiderBackend&) = default;
    IKernelFilesHiderBackend(IKernelFilesHiderBackend&&) = default;
    IKernelFilesHiderBackend& operator=(IKernelFilesHiderBackend&&) = default;
    virtual ~IKernelFilesHiderBackend() = default;

    virtual void hide_pattern(const char* pattern, size_t pattern_len) = 0;
    virtual void unhide_pattern(const char* pattern, size_t pattern_len) = 0;
    virtual void update_statistics() = 0;
};
