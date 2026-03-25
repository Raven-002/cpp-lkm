#include "cpp_lkm/module/module.hpp"

#include "cpp_lkm/runtime/kalloc.hpp"
#include "cpp_lkm/runtime/kernel_api.h"

class CppKernelModule::Resource
{
  public:
    int id = 0;
};

CppKernelModule::CppKernelModule()
{
    cpp_printk(CPP_KERN_INFO "[CPP] Constructed\n");
}

Result<void> CppKernelModule::init()
{
    auto assign_resource = [](Resource*& target) -> Result<void>
    {
        auto allocated = kalloc<Resource>();
        if (!allocated)
        {
            return std::unexpected(allocated.error());
        }
        target = *allocated;
        return {};
    };

    if (auto first = assign_resource(_resource1); !first)
    {
        return std::unexpected(first.error());
    }
    if (auto second = assign_resource(_resource2); !second)
    {
        return std::unexpected(second.error());
    }

    _resource1->id = 1;
    _resource2->id = 2;

    if (auto usr_init = _userspace.init(); !usr_init)
    {
        return usr_init;
    }

    cpp_printk(CPP_KERN_INFO "[CPP] Initialized\n");
    return {};
}

CppKernelModule::~CppKernelModule()
{
    kfree_obj(_resource1);
    kfree_obj(_resource2);
    cpp_printk(CPP_KERN_INFO "[CPP] Destructed\n");
}
