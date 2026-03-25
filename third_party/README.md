# third_party status

This directory is reserved for vendored dependencies if they are ever required.

At present, no third-party library is consumed from this tree. In particular,
`std::expected` is provided natively by the toolchain and enforced by
`cmake/cpp/CheckExpected.cmake`.
