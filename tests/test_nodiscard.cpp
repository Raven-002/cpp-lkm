#include "compat/expected.hpp"

[[nodiscard]] std::expected<int, int> returns_expected()
{
    return 1;
}

int main()
{
    returns_expected(); // Warning turns to error: ignoring return value of function declared with
                        // 'nodiscard' attribute
    return 0;
}
