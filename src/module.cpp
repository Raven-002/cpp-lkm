#include "module.hpp"

#include "kalloc.hpp"

#include <linux/kernel.h>

class CppKernelModule::Resource
{
  public:
    int id = 0;
};

CppKernelModule::CppKernelModule()
{
    printk(KERN_INFO "[CPP] Constructed\n");
}

std::expected<void, ErrorCode> CppKernelModule::init()
{
    auto assign_resource = [](Resource*& target) -> std::expected<void, ErrorCode>
    {
        auto allocated = kalloc<Resource>();
        if (!allocated)
            return std::unexpected(allocated.error());
        target = *allocated;
        return {};
    };

    if (auto first = assign_resource(_resource1); !first)
        return std::unexpected(first.error());
    if (auto second = assign_resource(_resource2); !second)
        return std::unexpected(second.error());

    _resource1->id = 1;
    _resource2->id = 2;

    printk(KERN_INFO "[CPP] Initialized\n");
    return {};
}

CppKernelModule::~CppKernelModule()
{
    kfree_obj(_resource1);
    kfree_obj(_resource2);
    printk(KERN_INFO "[CPP] Destructed\n");
}
