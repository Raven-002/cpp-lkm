#include "module.h"

#include "kalloc.h"

#include <linux/kernel.h>

CppKernelModule::CppKernelModule()
{
    printk(KERN_INFO "[CPP] Constructed\n");
}

[[nodiscard]] std::expected<void, ErrorCode> CppKernelModule::init()
{
    auto res1 = kalloc<TestResource>();
    if (!res1)
    {
        return std::unexpected(res1.error());
    }
    _resource1 = *res1;

    auto res2 = kalloc<TestResource>();
    if (!res2)
    {
        return std::unexpected(res2.error());
    }
    _resource2 = *res2;

    _resource1->id = 1;
    _resource2->id = 2;

    _initialized = true;
    printk(KERN_INFO "[CPP] Initialized\n");
    return {};
}

CppKernelModule::~CppKernelModule()
{
    kfree_obj(_resource1);
    kfree_obj(_resource2);
    printk(KERN_INFO "[CPP] Destructed\n");
}
