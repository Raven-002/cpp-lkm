#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/preempt.h>
#include <linux/hardirq.h>
#include <linux/irqflags.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <stdarg.h>

/* Forward declarations to satisfy -Wmissing-prototypes */
int cpp_printk(const char *fmt, ...);
void *cpp_kmalloc(size_t size, gfp_t flags);
void cpp_kfree(const void *p);
void cpp_assert_fail(const char *expr, const char *file, int line, const char *func);
void __assert_fail(const char *assertion, const char *file, int line, const char *function);
int cpp_in_atomic(void);
int cpp_irqs_disabled(void);
int cpp_in_nmi(void);

typedef s64 cpp_ssize_t;
typedef cpp_ssize_t (*cpp_chardev_read_cb)(void *ctx, void *kbuf, size_t len, s64 *pos);
typedef cpp_ssize_t (*cpp_chardev_write_cb)(void *ctx, const void *kbuf, size_t len,
                                            s64 *pos);

int cpp_userspace_chardev_register(const char *name, unsigned int mode, void *ctx,
                                   cpp_chardev_read_cb read_cb, cpp_chardev_write_cb write_cb);
void cpp_userspace_chardev_unregister(void);

// This file is compiled by Kbuild to provide the necessary
// kernel module metadata and entry points.

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Antigravity Agent");
MODULE_DESCRIPTION("C++ Kernel Module via CMake+Kbuild");

// --- C++ Bridge Implementations ---

int cpp_printk(const char *fmt, ...) {
    va_list args;
    int r;
    va_start(args, fmt);
    r = vprintk(fmt, args);
    va_end(args);
    return r;
}

void *cpp_kmalloc(size_t size, gfp_t flags) {
    return kmalloc(size, flags);
}

void cpp_kfree(const void *p) {
    kfree(p);
}

void cpp_assert_fail(const char *expr, const char *file, int line, const char *func) {
    panic("C++ BUG(): %s at %s:%d %s", expr, file, line, func);
}

/* Resolve C++ assert() calls; kernel has no libc __assert_fail */
void __assert_fail(const char *assertion, const char *file, int line, const char *function) {
    cpp_assert_fail(assertion, file, line, function);
}

int cpp_in_atomic(void) {
    return in_atomic();
}

int cpp_irqs_disabled(void) {
    return irqs_disabled();
}

int cpp_in_nmi(void) {
    return in_nmi();
}

// --- Userspace char device (miscdevice) bridge ---

static char cpp_lkm_name_buf[64];
static cpp_chardev_read_cb g_read_cb;
static cpp_chardev_write_cb g_write_cb;
static void *g_chardev_ctx;
static int g_chardev_registered;

static ssize_t cpp_lkm_read(struct file *file, char __user *buf, size_t len, loff_t *pos);
static ssize_t cpp_lkm_write(struct file *file, const char __user *buf, size_t len, loff_t *pos);

static const struct file_operations cpp_lkm_fops = {
    .owner = THIS_MODULE,
    .read = cpp_lkm_read,
    .write = cpp_lkm_write,
};

static struct miscdevice cpp_lkm_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .fops = &cpp_lkm_fops,
};

static ssize_t cpp_lkm_read(struct file *file, char __user *buf, size_t len, loff_t *pos) {
    char *kbuf;
    ssize_t ret;
    size_t chunk;
    s64 pos_ll;

    (void)file;
    if (!g_read_cb || !g_chardev_ctx)
        return -EINVAL;
    if (len == 0)
        return 0;

    chunk = len < 4096u ? len : 4096u;
    kbuf = kmalloc(chunk, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    pos_ll = (s64)*pos;
    ret = g_read_cb(g_chardev_ctx, kbuf, chunk, &pos_ll);
    if (ret < 0) {
        kfree(kbuf);
        return ret;
    }
    if (copy_to_user(buf, kbuf, (size_t)ret)) {
        kfree(kbuf);
        return -EFAULT;
    }
    *pos += ret;
    kfree(kbuf);
    return ret;
}

static ssize_t cpp_lkm_write(struct file *file, const char __user *buf, size_t len,
                               loff_t *pos) {
    char *kbuf;
    ssize_t ret;
    size_t chunk;
    s64 pos_ll;

    (void)file;
    if (!g_write_cb || !g_chardev_ctx)
        return -EINVAL;
    if (len == 0)
        return 0;

    chunk = len < 4096u ? len : 4096u;
    kbuf = kmalloc(chunk, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    if (copy_from_user(kbuf, buf, chunk)) {
        kfree(kbuf);
        return -EFAULT;
    }

    pos_ll = (s64)*pos;
    ret = g_write_cb(g_chardev_ctx, kbuf, chunk, &pos_ll);
    if (ret < 0) {
        kfree(kbuf);
        return ret;
    }
    *pos += ret;
    kfree(kbuf);
    return ret;
}

int cpp_userspace_chardev_register(const char *name, unsigned int mode, void *ctx,
                                   cpp_chardev_read_cb read_cb, cpp_chardev_write_cb write_cb) {
    int err;

    if (g_chardev_registered)
        return -EBUSY;
    if (!name || !read_cb || !write_cb || !ctx)
        return -EINVAL;

    strscpy(cpp_lkm_name_buf, name, sizeof(cpp_lkm_name_buf));
    g_chardev_ctx = ctx;
    g_read_cb = read_cb;
    g_write_cb = write_cb;

    cpp_lkm_misc.name = cpp_lkm_name_buf;
    cpp_lkm_misc.mode = mode;

    err = misc_register(&cpp_lkm_misc);
    if (err) {
        g_chardev_ctx = NULL;
        g_read_cb = NULL;
        g_write_cb = NULL;
        return err;
    }
    g_chardev_registered = 1;
    return 0;
}

void cpp_userspace_chardev_unregister(void) {
    if (!g_chardev_registered)
        return;
    misc_deregister(&cpp_lkm_misc);
    g_chardev_registered = 0;
    g_chardev_ctx = NULL;
    g_read_cb = NULL;
    g_write_cb = NULL;
}

// --- Module lifecycle ---

// These will be resolved to the C++ entry points in bridge.cpp
extern int  cpp_module_init(void);
extern void cpp_module_exit(void);

static int __init lkm_init(void) {
    return cpp_module_init();
}

static void __exit lkm_exit(void) {
    cpp_module_exit();
}

module_init(lkm_init);
module_exit(lkm_exit);
