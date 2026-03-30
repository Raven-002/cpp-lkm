#include "kernel_module/kernel_module.hpp"

#include "kernel_api/kernel_api.h"

KernelModule::KernelModule() : _userspace(_echo_server)
{
    cpp_printk(CPP_KERN_INFO "[CPP] Constructed\n");
}

Result<void> KernelModule::init()
{
    auto usr_init = _userspace.init();
    if (!usr_init)
    {
        return usr_init;
    }

    cpp_printk(CPP_KERN_INFO "[CPP] Initialized\n");
    return {};
}

KernelModule::~KernelModule()
{
    cpp_printk(CPP_KERN_INFO "[CPP] Destructed\n");
}
