// include/test_resource.hpp
// A minimal resource type used to demonstrate two-phase allocation/cleanup
// in CppKernelModule. Not a kernel type — demo/test purposes only.
#pragma once

struct TestResource
{
    int id = 0;
};
