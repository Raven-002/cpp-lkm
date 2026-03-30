#include "kernel_module/features/files_hider/dummy_kernel_files_hider_backend.hpp"

#include "kernel_api/kernel_api.h"

void DummyKernelFilesHiderBackend::hide_pattern(std::string_view pattern,
                                                IKernelFilesHiderStatsSink& stats_sink)
{
    _stats_sink = &stats_sink;
    cpp_printk(CPP_KERN_INFO "[CPP] FilesHider hide_pattern_hook called for: %.*s\n",
               static_cast<int>(pattern.size()), pattern.data());
}

void DummyKernelFilesHiderBackend::unhide_pattern(std::string_view pattern)
{
    cpp_printk(CPP_KERN_INFO "[CPP] FilesHider unhide_pattern_hook called for: %.*s\n",
               static_cast<int>(pattern.size()), pattern.data());
}
