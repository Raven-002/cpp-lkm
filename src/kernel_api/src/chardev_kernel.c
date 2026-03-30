// Misc character device bridge — implements cpp_userspace_chardev_* from
// kernel_api/kernel_api.h. Kept as C: kernel mutex / kvmalloc / file_operations
// patterns are not reliably C++-compatible on recent kernels.

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include "kernel_api/kernel_api.h"

struct cpp_chardev_entry
{
    struct miscdevice miscdev;
    void* ctx;
    cpp_chardev_read_cb read_cb;
    cpp_chardev_write_cb write_cb;
    struct list_head list;
};

static DEFINE_MUTEX(g_chardev_lock);
static LIST_HEAD(g_chardev_entries);

static struct cpp_chardev_entry* cpp_chardev_from_file(const struct file* file)
{
    if (file == NULL || file->private_data == NULL)
    {
        return NULL;
    }

    return container_of((struct miscdevice*)file->private_data, struct cpp_chardev_entry, miscdev);
}

static struct cpp_chardev_entry* cpp_chardev_find_by_ctx(void* ctx)
{
    struct cpp_chardev_entry* entry = NULL;

    list_for_each_entry(entry, &g_chardev_entries, list)
    {
        if (entry->ctx == ctx)
        {
            return entry;
        }
    }

    return NULL;
}

static ssize_t cpp_chardev_read(struct file* file, char __user* ubuf, size_t len, loff_t* ppos)
{
    struct cpp_chardev_entry* entry = NULL;
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
    mutex_lock(&g_chardev_lock);
    entry = cpp_chardev_from_file(file);
    if (entry == NULL || entry->read_cb == NULL || entry->ctx == NULL)
    {
        mutex_unlock(&g_chardev_lock);
        return -ENODEV;
    }

    kbuf = kvmalloc(len, GFP_KERNEL);
    if (kbuf == NULL)
    {
        mutex_unlock(&g_chardev_lock);
        return -ENOMEM;
    }

    pos = (int64_t)(*ppos);
    nread = entry->read_cb(entry->ctx, kbuf, len, &pos);
    if (nread > 0)
    {
        if ((size_t)nread > len)
        {
            kvfree(kbuf);
            mutex_unlock(&g_chardev_lock);
            return -EIO;
        }
        if (copy_to_user(ubuf, kbuf, (size_t)nread) != 0U)
        {
            kvfree(kbuf);
            mutex_unlock(&g_chardev_lock);
            return -EFAULT;
        }
        *ppos = (loff_t)pos;
    }

    kvfree(kbuf);
    mutex_unlock(&g_chardev_lock);
    return (ssize_t)nread;
}

static ssize_t cpp_chardev_write(struct file* file, const char __user* ubuf, size_t len,
                                 loff_t* ppos)
{
    struct cpp_chardev_entry* entry = NULL;
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
    mutex_lock(&g_chardev_lock);
    entry = cpp_chardev_from_file(file);
    if (entry == NULL || entry->write_cb == NULL || entry->ctx == NULL)
    {
        mutex_unlock(&g_chardev_lock);
        return -ENODEV;
    }

    kbuf = kvmalloc(len, GFP_KERNEL);
    if (kbuf == NULL)
    {
        mutex_unlock(&g_chardev_lock);
        return -ENOMEM;
    }
    if (copy_from_user(kbuf, ubuf, len) != 0U)
    {
        kvfree(kbuf);
        mutex_unlock(&g_chardev_lock);
        return -EFAULT;
    }

    pos = (int64_t)(*ppos);
    nwritten = entry->write_cb(entry->ctx, kbuf, len, &pos);
    if (nwritten >= 0)
    {
        *ppos = (loff_t)pos;
    }

    kvfree(kbuf);
    mutex_unlock(&g_chardev_lock);
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
    struct cpp_chardev_entry* entry = NULL;
    int err = 0;

    if (name == NULL || ctx == NULL || read_cb == NULL || write_cb == NULL)
    {
        return -EINVAL;
    }

    mutex_lock(&g_chardev_lock);
    if (cpp_chardev_find_by_ctx(ctx) != NULL)
    {
        mutex_unlock(&g_chardev_lock);
        return -EEXIST;
    }

    entry = kvmalloc(sizeof(*entry), GFP_KERNEL);
    if (entry == NULL)
    {
        mutex_unlock(&g_chardev_lock);
        return -ENOMEM;
    }

    entry->ctx = ctx;
    entry->read_cb = read_cb;
    entry->write_cb = write_cb;
    entry->miscdev.minor = MISC_DYNAMIC_MINOR;
    entry->miscdev.name = name;
    entry->miscdev.fops = &g_cpp_fops;
    entry->miscdev.mode = (umode_t)mode;

    err = misc_register(&entry->miscdev);
    if (err != 0)
    {
        kvfree(entry);
        mutex_unlock(&g_chardev_lock);
        return err;
    }

    INIT_LIST_HEAD(&entry->list);
    list_add_tail(&entry->list, &g_chardev_entries);
    mutex_unlock(&g_chardev_lock);
    return 0;
}

void cpp_userspace_chardev_unregister(void* ctx)
{
    struct cpp_chardev_entry* entry = NULL;

    if (ctx == NULL)
    {
        return;
    }

    mutex_lock(&g_chardev_lock);
    entry = cpp_chardev_find_by_ctx(ctx);
    if (entry != NULL)
    {
        list_del(&entry->list);
        misc_deregister(&entry->miscdev);
        kvfree(entry);
    }
    mutex_unlock(&g_chardev_lock);
}
