#include "module.hpp"

#include <linux/module.h>

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
            return static_cast<int>(res.error());
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
