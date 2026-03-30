#pragma once

#include <string_view>

class IKernelFilesHiderStatsSink
{
  public:
    IKernelFilesHiderStatsSink() = default;
    IKernelFilesHiderStatsSink(const IKernelFilesHiderStatsSink&) = default;
    IKernelFilesHiderStatsSink& operator=(const IKernelFilesHiderStatsSink&) = default;
    IKernelFilesHiderStatsSink(IKernelFilesHiderStatsSink&&) = default;
    IKernelFilesHiderStatsSink& operator=(IKernelFilesHiderStatsSink&&) = default;
    virtual ~IKernelFilesHiderStatsSink() = default;

    virtual void record_hide_applied(std::string_view pattern) = 0;
    virtual void record_hide_skipped(std::string_view pattern) = 0;
};

class IKernelFilesHiderBackend
{
  public:
    IKernelFilesHiderBackend() = default;
    IKernelFilesHiderBackend(const IKernelFilesHiderBackend&) = default;
    IKernelFilesHiderBackend& operator=(const IKernelFilesHiderBackend&) = default;
    IKernelFilesHiderBackend(IKernelFilesHiderBackend&&) = default;
    IKernelFilesHiderBackend& operator=(IKernelFilesHiderBackend&&) = default;
    virtual ~IKernelFilesHiderBackend() = default;

    virtual void hide_pattern(std::string_view pattern, IKernelFilesHiderStatsSink& stats_sink) = 0;
    virtual void unhide_pattern(std::string_view pattern) = 0;
};
