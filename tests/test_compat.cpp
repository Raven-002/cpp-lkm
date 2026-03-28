#include "cpp_lkm/common/error.hpp"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <expected>

static void test_expected_value()
{
    std::expected<int, int> expected_ok{42};
    assert(expected_ok.has_value());
    assert(expected_ok.value() == 42);
    printf("test_expected_value passed\n");
}

static void test_expected_error()
{
    Result<int> err_result = std::unexpected(-ENOMEM);
    assert(!err_result.has_value());
    assert(err_result.error() == -ENOMEM);
    printf("test_expected_error passed\n");
}

static void test_expected_monadic()
{
    std::expected<int, int> base{21};
    auto doubled = base.and_then([](int val) -> std::expected<int, int> { return val * 2; });
    assert(doubled.has_value() && doubled.value() == 42);

    auto failed = doubled.and_then([](int /*unused*/) -> std::expected<int, int>
                                   { return std::unexpected(10); });
    assert(!failed.has_value() && failed.error() == 10);

    printf("test_expected_monadic passed\n");
}

extern "C" int main()
{
    test_expected_value();
    test_expected_error();
    test_expected_monadic();
    printf("All compat tests passed!\n");
    return 0;
}
