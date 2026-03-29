#include "kernel_module/kernel_module.hpp"

#include "cpp_lkm/runtime/kernel_api.h"

#include <utility>

class KernelModule::Resource
{
  public:
    int id = 0;
};

KernelModule::KernelModule() : _userspace(_echo_server)
{
    cpp_printk(CPP_KERN_INFO "[CPP] Constructed\n");
}

Result<void> KernelModule::init()
{
    auto resource1 = kalloc_owned<Resource>();
    if (!resource1)
    {
        return std::unexpected(resource1.error());
    }

    auto resource2 = kalloc_owned<Resource>();
    if (!resource2)
    {
        return std::unexpected(resource2.error());
    }

    _resource1 = std::move(*resource1);
    _resource2 = std::move(*resource2);

    _resource1->id = 1;
    _resource2->id = 2;

    if (auto usr_init = _userspace.init(); !usr_init)
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
