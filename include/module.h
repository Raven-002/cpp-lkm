#pragma once
#include "compat/expected.h"
#include "error.h"

// A dummy resource used to demonstrate allocation and cleanup.
struct TestResource {
    int id = 0;
};

class CppKernelModule {
public:
    CppKernelModule();
    [[nodiscard]] std::expected<void, ErrorCode> init();
    ~CppKernelModule();

private:
    bool _initialized = false;
    TestResource* _resource1 = nullptr;
    TestResource* _resource2 = nullptr;
};
