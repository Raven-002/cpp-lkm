#include "error.hpp"

#include <assert.h>
#include <expected>
#include <stdio.h>

void test_expected_value()
{
    std::expected<int, int> e{42};
    assert(e.has_value());
    assert(e.value() == 42);
    printf("test_expected_value passed\n");
}

void test_expected_error()
{
    std::expected<int, ErrorCode> e = std::unexpected(ErrorCode::AllocFail);
    assert(!e.has_value());
    assert(e.error() == ErrorCode::AllocFail);
    printf("test_expected_error passed\n");
}

void test_expected_monadic()
{
    std::expected<int, int> e{21};
    auto r = e.and_then([](int v) -> std::expected<int, int> { return v * 2; });
    assert(r.has_value() && r.value() == 42);

    auto r2 = r.and_then([](int) -> std::expected<int, int> { return std::unexpected(10); });
    assert(!r2.has_value() && r2.error() == 10);

    printf("test_expected_monadic passed\n");
}

int main()
{
    test_expected_value();
    test_expected_error();
    test_expected_monadic();
    printf("All compat tests passed!\n");
    return 0;
}
