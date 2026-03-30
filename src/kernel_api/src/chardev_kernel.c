// Misc character device bridge — implements cpp_userspace_chardev_* from
// kernel_api/kernel_api.h. Kept as C: kernel mutex / kvmalloc / file_operations
// patterns are not reliably C++-compatible on recent kernels.

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include "kernel_api/kernel_api.h"

static DEFINE_MUTEX(g_chardev_lock);
static struct miscdevice g_cpp_miscdev;
static void* g_chardev_ctx;
static cpp_chardev_read_cb g_chardev_read_cb;
static cpp_chardev_write_cb g_chardev_write_cb;
static int g_chardev_registered;

static ssize_t cpp_chardev_read(struct file* file, char __user* ubuf, size_t len, loff_t* ppos)
{
    void* kbuf = NULL;
    int64_t pos = 0;
    cpp_ssize_t nread = 0;

    (void)file;

    if (ubuf == NULL || ppos == NULL)
    {
        return -EINVAL;
    }
    if (len == 0)
    {
        return 0;
    }
    if (g_chardev_read_cb == NULL || g_chardev_ctx == NULL)
    {
        return -ENODEV;
    }

    kbuf = kvmalloc(len, GFP_KERNEL);
    if (kbuf == NULL)
    {
        return -ENOMEM;
    }

    pos = (int64_t)(*ppos);
    nread = g_chardev_read_cb(g_chardev_ctx, kbuf, len, &pos);
    if (nread > 0)
    {
        if ((size_t)nread > len)
        {
            kvfree(kbuf);
            return -EIO;
        }
        if (copy_to_user(ubuf, kbuf, (size_t)nread) != 0U)
        {
            kvfree(kbuf);
            return -EFAULT;
        }
        *ppos = (loff_t)pos;
    }

    kvfree(kbuf);
    return (ssize_t)nread;
}

static ssize_t cpp_chardev_write(struct file* file, const char __user* ubuf, size_t len,
                                 loff_t* ppos)
{
    void* kbuf = NULL;
    int64_t pos = 0;
    cpp_ssize_t nwritten = 0;

    (void)file;

    if (ubuf == NULL || ppos == NULL)
    {
        return -EINVAL;
    }
    if (len == 0)
    {
        return 0;
    }
    if (g_chardev_write_cb == NULL || g_chardev_ctx == NULL)
    {
        return -ENODEV;
    }

    kbuf = kvmalloc(len, GFP_KERNEL);
    if (kbuf == NULL)
    {
        return -ENOMEM;
    }
    if (copy_from_user(kbuf, ubuf, len) != 0U)
    {
        kvfree(kbuf);
        return -EFAULT;
    }

    pos = (int64_t)(*ppos);
    nwritten = g_chardev_write_cb(g_chardev_ctx, kbuf, len, &pos);
    if (nwritten >= 0)
    {
        *ppos = (loff_t)pos;
    }

    kvfree(kbuf);
    return (ssize_t)nwritten;
}

static const struct file_operations g_cpp_fops = {
    .owner = THIS_MODULE,
    .read = cpp_chardev_read,
    .write = cpp_chardev_write,
};

int cpp_userspace_chardev_register(const char* name, unsigned int mode, void* ctx,
                                   cpp_chardev_read_cb read_cb, cpp_chardev_write_cb write_cb)
{
    int err = 0;

    if (name == NULL || read_cb == NULL || write_cb == NULL)
    {
        return -EINVAL;
    }

    mutex_lock(&g_chardev_lock);
    if (g_chardev_registered != 0)
    {
        mutex_unlock(&g_chardev_lock);
        return -EBUSY;
    }

    g_chardev_ctx = ctx;
    g_chardev_read_cb = read_cb;
    g_chardev_write_cb = write_cb;
    g_cpp_miscdev.minor = MISC_DYNAMIC_MINOR;
    g_cpp_miscdev.name = name;
    g_cpp_miscdev.fops = &g_cpp_fops;
    g_cpp_miscdev.mode = (umode_t)mode;

    err = misc_register(&g_cpp_miscdev);
    if (err != 0)
    {
        g_chardev_ctx = NULL;
        g_chardev_read_cb = NULL;
        g_chardev_write_cb = NULL;
        mutex_unlock(&g_chardev_lock);
        return err;
    }

    g_chardev_registered = 1;
    mutex_unlock(&g_chardev_lock);
    return 0;
}

void cpp_userspace_chardev_unregister(void)
{
    mutex_lock(&g_chardev_lock);
    if (g_chardev_registered != 0)
    {
        misc_deregister(&g_cpp_miscdev);
        g_chardev_registered = 0;
    }
    g_chardev_ctx = NULL;
    g_chardev_read_cb = NULL;
    g_chardev_write_cb = NULL;
    mutex_unlock(&g_chardev_lock);
}
