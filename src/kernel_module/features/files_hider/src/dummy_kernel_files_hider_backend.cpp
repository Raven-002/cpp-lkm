#include "kernel_module/features/files_hider/dummy_kernel_files_hider_backend.hpp"

#include "kernel_api/kernel_api.h"

#include <limits>

void DummyKernelFilesHiderBackend::hide_pattern(const char* pattern, size_t pattern_len)
{
    const auto max_log_len = static_cast<size_t>(std::numeric_limits<int>::max());
    const int log_len = static_cast<int>(pattern_len < max_log_len ? pattern_len : max_log_len);
    cpp_printk(CPP_KERN_INFO "[CPP] FilesHider hide_pattern_hook called for: %.*s\n", log_len,
               pattern);
}

void DummyKernelFilesHiderBackend::unhide_pattern(const char* pattern, size_t pattern_len)
{
    const auto max_log_len = static_cast<size_t>(std::numeric_limits<int>::max());
    const int log_len = static_cast<int>(pattern_len < max_log_len ? pattern_len : max_log_len);
    cpp_printk(CPP_KERN_INFO "[CPP] FilesHider unhide_pattern_hook called for: %.*s\n", log_len,
               pattern);
}

void DummyKernelFilesHiderBackend::update_statistics()
{
    cpp_printk(CPP_KERN_INFO "[CPP] FilesHider update_statistics_hook called\n");
}
