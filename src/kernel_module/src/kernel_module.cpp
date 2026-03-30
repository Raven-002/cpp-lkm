#include "kernel_module/kernel_module.hpp"

#include "kernel_api/kernel_api.h"

KernelModule::KernelModule()
    : _echo_userspace(_echo_server, "cpp_lkm"),
      _files_hider_userspace(_files_hider_server, "cpp_lkm_files_hider")
{
    cpp_printk(CPP_KERN_INFO "[CPP] Constructed\n");
}

Result<void> KernelModule::init()
{
    auto echo_init = _echo_userspace.init();
    if (!echo_init)
    {
        return echo_init;
    }

    auto files_hider_init = _files_hider_userspace.init();
    if (!files_hider_init)
    {
        return files_hider_init;
    }

    cpp_printk(CPP_KERN_INFO "[CPP] Initialized\n");
    return {};
}

KernelModule::~KernelModule()
{
    cpp_printk(CPP_KERN_INFO "[CPP] Destructed\n");
}
