#pragma once

// gsl::owner matches Microsoft GSL and clang-tidy cppcoreguidelines-owning-memory.
// It is a type alias only; no runtime change.
namespace gsl
{
template <class T>
using owner = T;
} // namespace gsl
