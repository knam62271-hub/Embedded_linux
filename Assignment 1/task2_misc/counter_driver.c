#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>

#define DEVICE_NAME "counter"
#define PROC_NAME "counter_info"

static atomic_t counter = ATOMIC_INIT(0);
static atomic_t total_reads = ATOMIC_INIT(0);
static atomic_t total_writes = ATOMIC_INIT(0);

static DEFINE_MUTEX(counter_mutex);

static struct proc_dir_entry *counter_proc_entry;

/* Read current counter and increment it */
static ssize_t counter_read(struct file *file,
                            char __user *buf,
                            size_t count,
                            loff_t *ppos)
{
    char data[32];
    int value;
    int len;

    if (*ppos > 0)
        return 0;

    if (count == 0)
        return 0;

    mutex_lock(&counter_mutex);

    value = atomic_read(&counter);
    len = scnprintf(data, sizeof(data), "%d\n", value);

    if (count < len) {
        mutex_unlock(&counter_mutex);
        return -EINVAL;
    }

    if (copy_to_user(buf, data, len)) {
        mutex_unlock(&counter_mutex);
        return -EFAULT;
    }

    *ppos += len;

    atomic_inc(&counter);
    atomic_inc(&total_reads);

    mutex_unlock(&counter_mutex);

    return len;
}

/* Set counter from userspace */
static ssize_t counter_write(struct file *file,
                             const char __user *buf,
                             size_t count,
                             loff_t *ppos)
{
    char data[32];
    int value;

    if (count == 0 || count >= sizeof(data))
        return -EINVAL;

    if (copy_from_user(data, buf, count))
        return -EFAULT;

    data[count] = '\0';

    if (kstrtoint(data, 10, &value) < 0 || value < 0)
        return -EINVAL;

    mutex_lock(&counter_mutex);

    atomic_set(&counter, value);
    atomic_inc(&total_writes);

    mutex_unlock(&counter_mutex);

    return count;
}

/* Display counter statistics */
static int counter_proc_show(struct seq_file *m, void *v)
{
    mutex_lock(&counter_mutex);

    seq_printf(m, "Counter Information\n");
    seq_printf(m, "-------------------\n");
    seq_printf(m, "Current counter : %d\n",
               atomic_read(&counter));
    seq_printf(m, "Total reads     : %d\n",
               atomic_read(&total_reads));
    seq_printf(m, "Total writes    : %d\n",
               atomic_read(&total_writes));

    mutex_unlock(&counter_mutex);

    return 0;
}

static int counter_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, counter_proc_show, NULL);
}

/* Device file operations */
static const struct file_operations counter_fops = {
    .owner = THIS_MODULE,
    .read = counter_read,
    .write = counter_write,
    .llseek = no_llseek,
};

/* Misc device registration */
static struct miscdevice counter_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &counter_fops,
};

/* Proc file operations */
static const struct proc_ops counter_proc_ops = {
    .proc_open = counter_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/* Initialize counter driver */
static int __init counter_init(void)
{
    int ret;

    atomic_set(&counter, 0);
    atomic_set(&total_reads, 0);
    atomic_set(&total_writes, 0);

    ret = misc_register(&counter_device);
    if (ret) {
        pr_err("counter_driver: misc_register failed\n");
        return ret;
    }

    counter_proc_entry = proc_create(
        PROC_NAME, 0444, NULL, &counter_proc_ops);

    if (!counter_proc_entry) {
        misc_deregister(&counter_device);
        pr_err("counter_driver: proc_create failed\n");
        return -ENOMEM;
    }

    pr_info("counter_driver: loaded successfully\n");

    return 0;
}

/* Remove counter driver */
static void __exit counter_exit(void)
{
    proc_remove(counter_proc_entry);
    misc_deregister(&counter_device);

    pr_info("counter_driver: unloaded\n");
}

module_init(counter_init);
module_exit(counter_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nguyen Nam Khanh - SE205204");
MODULE_DESCRIPTION("Simple Misc Counter Device Driver");
MODULE_VERSION("1.0");
