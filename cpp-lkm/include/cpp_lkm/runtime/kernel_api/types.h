#pragma once

#ifdef __cplusplus
#include <cstddef>

#ifndef CPP_LKM_GFP_TYPES_DEFINED
#define CPP_LKM_GFP_TYPES_DEFINED
using cpp_gfp_t = unsigned int;
inline constexpr cpp_gfp_t CPP_GFP_KERNEL = 0x1U;
inline constexpr cpp_gfp_t CPP_GFP_ATOMIC = 0x2U;
#endif
#else
#ifdef __KERNEL__
#include <linux/stddef.h>
#else
#include <stddef.h>
#endif

#ifndef CPP_LKM_GFP_TYPES_DEFINED
#define CPP_LKM_GFP_TYPES_DEFINED
typedef unsigned int cpp_gfp_t;
#define CPP_GFP_KERNEL 0x1U
#define CPP_GFP_ATOMIC 0x2U
#endif
#endif
