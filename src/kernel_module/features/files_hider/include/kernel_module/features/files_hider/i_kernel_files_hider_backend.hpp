#pragma once

#include <cstddef>

class IKernelFilesHiderStatsSink
{
  public:
    IKernelFilesHiderStatsSink() = default;
    IKernelFilesHiderStatsSink(const IKernelFilesHiderStatsSink&) = default;
    IKernelFilesHiderStatsSink& operator=(const IKernelFilesHiderStatsSink&) = default;
    IKernelFilesHiderStatsSink(IKernelFilesHiderStatsSink&&) = default;
    IKernelFilesHiderStatsSink& operator=(IKernelFilesHiderStatsSink&&) = default;
    virtual ~IKernelFilesHiderStatsSink() = default;

    // Call when a pattern match resulted in an actual hide.
    virtual void record_hide_applied(const char* pattern, size_t pattern_len) = 0;
    // Call when a pattern match happened but hide was skipped (e.g. PID guard).
    virtual void record_hide_skipped(const char* pattern, size_t pattern_len) = 0;
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

    // Register or update a hidden pattern. Backends should keep stats_sink available
    // in the hide-time path and report hide_applied/hide_skipped for each matched pattern.
    virtual void hide_pattern(const char* pattern, size_t pattern_len,
                              IKernelFilesHiderStatsSink& stats_sink) = 0;
    virtual void unhide_pattern(const char* pattern, size_t pattern_len) = 0;
};
