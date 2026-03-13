#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern int  cpp_module_init(void);
extern void cpp_module_exit(void);

#define module_init(fn)
#define module_exit(fn)
#define MODULE_LICENSE(s)
#define MODULE_AUTHOR(s)
#define MODULE_DESCRIPTION(s)

#ifdef __cplusplus
}
#endif
