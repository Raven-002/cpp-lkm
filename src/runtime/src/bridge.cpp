#include "compat/new_shim.hpp" // IWYU pragma: keep
#include "cpp_lkm/common/error.hpp"
#include "cpp_lkm/module/module.hpp"
#include "cpp_lkm/runtime/kernel_api.h"

#include <array>

// NOLINTBEGIN(cert-dcl58-cpp)
namespace std
{
void __glibcxx_assert_fail(const char* file, int line, const char* function,
                           const char* condition) noexcept
{
    cpp_printk(CPP_KERN_ERR "[CPP] ASSERTION FAILED: %s:%d in %s: %s\n", file, line, function,
               condition);
    while (true)
    {
    } // Halting execution without calling the bug macro
}
} // namespace std
// NOLINTEND(cert-dcl58-cpp)

#include "cpp_lkm/runtime/module_entry.h"

namespace
{
struct alignas(CppKernelModule) ModuleStorage
{
    std::array<unsigned char, sizeof(CppKernelModule)> bytes{};
};
// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
ModuleStorage g_module_storage{};
CppKernelModule* g_module = nullptr;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

static_assert(sizeof(ModuleStorage::bytes) >= sizeof(CppKernelModule),
              "ModuleStorage size must satisfy module size");
static_assert(alignof(ModuleStorage) >= alignof(CppKernelModule),
              "ModuleStorage alignment must satisfy module alignment");
} // namespace

extern "C"
{

    int cpp_module_init(void)
    {
        g_module = new (g_module_storage.bytes.data()) CppKernelModule{};
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
        if (g_module != nullptr)
        {
            g_module->~CppKernelModule();
            g_module = nullptr;
        }
    }

} // extern "C"
