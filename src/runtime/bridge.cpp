#include "cpp_lkm/common/error.hpp"
#include "cpp_lkm/module/module.hpp"

#include "cpp_lkm/runtime/module_entry.h"

extern "C"
{

    alignas(CppKernelModule) static unsigned char g_module_buf[sizeof(CppKernelModule)];
    static CppKernelModule* g_module = nullptr;

    int cpp_module_init(void)
    {
        g_module = new (g_module_buf) CppKernelModule{};
        auto res = g_module->init();
        if (!res)
        {
            g_module->~CppKernelModule();
            g_module = nullptr;
            return to_errno(res.error());
        }
        return 0;
    }

    void cpp_module_exit(void)
    {
        if (g_module)
        {
            g_module->~CppKernelModule();
            g_module = nullptr;
        }
    }

} // extern "C"
