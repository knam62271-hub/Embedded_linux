#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/ctype.h>
#include "asgn1_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Lab");
MODULE_DESCRIPTION("ASSIGNMENT-01: ioctl Character Device Driver");
MODULE_VERSION("1.0");

#define DEVICE_NAME "asgn1"
#define CLASS_NAME "asgn1_class"
#define BUFFER_SIZE 1024
#define DRIVER_MAJOR 241

/* 240-254: reserved for local/experimental use */

static int major_number = DRIVER_MAJOR;
static char device_buffer[BUFFER_SIZE];
static int buffer_len = 0;
static int open_count = 0;
static int read_count = 0;
static int read_only = 0;
static int device_open = 0;
static int write_count = 0;
static int last_write_size = 0;
static int mode = 1;


static struct class *lab2_class = NULL;
static struct device *lab2_device = NULL;
static DEFINE_MUTEX(lab2_mutex);

/* ===== SYSFS Implementation ===== */
static ssize_t buffer_len_show(struct device *dev,
 struct device_attribute *attr, char *buf)
{
    int value;

    mutex_lock(&lab2_mutex);
    value = buffer_len;
    mutex_unlock(&lab2_mutex);

    return sprintf(buf, "%d\n", value);
}

static ssize_t open_count_show(struct device *dev,
 struct device_attribute *attr, char *buf)
{
    int value;

    mutex_lock(&lab2_mutex);
    value = open_count;
    mutex_unlock(&lab2_mutex);

    return sprintf(buf, "%d\n", value);
}

static ssize_t last_data_show(struct device *dev,
 struct device_attribute *attr, char *buf)
{
    ssize_t result;

    mutex_lock(&lab2_mutex);

    if (buffer_len > 0)
        result = sprintf(buf, "%s\n", device_buffer);
    else
        result = sprintf(buf, "(empty)\n");

    mutex_unlock(&lab2_mutex);

    return result;
}

static ssize_t read_only_show(struct device *dev,
 struct device_attribute *attr, char *buf){
int value;
mutex_lock(&lab2_mutex);
value = read_only;
mutex_unlock(&lab2_mutex);
return sprintf(buf, "%d\n", value);
}
static ssize_t read_only_store(struct device *dev,
struct device_attribute *attr,
const char *buf,
size_t count)
{
    int value;
    if (kstrtoint(buf, 10, &value) != 0)
        return -EINVAL;
    if (value != 0 && value != 1)
        return -EINVAL;
    mutex_lock(&lab2_mutex);
    read_only = value;
    mutex_unlock(&lab2_mutex);
    return count;
}
static ssize_t word_count_show(struct device *dev,
struct device_attribute *attr,char *buf){
int count = 0;
int in_word = 0;
int i;
mutex_lock(&lab2_mutex);
for (i = 0; i < buffer_len; i++) {
if (isspace((unsigned char)device_buffer[i])) {
in_word = 0;
} else if (!in_word) {
count++;
in_word = 1;}
}
mutex_unlock(&lab2_mutex);
return sprintf(buf, "%d\n", count);
}
static DEVICE_ATTR_RO(buffer_len);
static DEVICE_ATTR_RO(open_count);
static DEVICE_ATTR_RO(last_data);
static DEVICE_ATTR_RW(read_only);
static DEVICE_ATTR_RO(word_count);
static struct attribute *lab2_attrs[] = {
 &dev_attr_buffer_len.attr,
 &dev_attr_open_count.attr,
 &dev_attr_last_data.attr,
 &dev_attr_read_only.attr,
 &dev_attr_word_count.attr,
 NULL,
};
static struct attribute_group lab2_attr_group = {
 .attrs = lab2_attrs,
};
/* ===== PROCFS Implementation ===== */
static struct proc_dir_entry *proc_entry;

static int lab2_proc_show(struct seq_file *m, void *v)
{
    mutex_lock(&lab2_mutex);

    seq_printf(m, "ASSIGNMENT-01 Driver Statistics\n");
    seq_printf(m, "==============================\n");
    seq_printf(m, "Driver name   : %s\n", DEVICE_NAME);
    seq_printf(m, "Major number  : %d\n", major_number);
    seq_printf(m, "Buffer size   : %d bytes\n", BUFFER_SIZE);
    seq_printf(m, "Data length   : %d bytes\n", buffer_len);
    seq_printf(m, "Open count    : %d\n", open_count);
    seq_printf(m, "Read count    : %d\n", read_count);
    seq_printf(m, "Write count   : %d\n", write_count);
    seq_printf(m, "Last write    : %d bytes\n", last_write_size);
    seq_printf(m, "Last data     : [%s]\n", device_buffer);

    mutex_unlock(&lab2_mutex);

    return 0;
}

static int lab2_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, lab2_proc_show, NULL);
}

static const struct proc_ops lab2_proc_fops = {
    .proc_open    = lab2_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int lab2_open(struct inode *inode, struct file *file)
{
    mutex_lock(&lab2_mutex);
    if (device_open) {
        mutex_unlock(&lab2_mutex);
        return -EBUSY;
    }
    device_open = 1;
    open_count++;
    mutex_unlock(&lab2_mutex);
    pr_info("asgn1_driver: device opened\n");
    return 0;
}

static int lab2_release(struct inode *inode, struct file *file)
{
    mutex_lock(&lab2_mutex);
    device_open = 0;
    mutex_unlock(&lab2_mutex);
    pr_info("asgn1_driver: device released\n");
    return 0;
}

static ssize_t lab2_read(struct file *file, char __user *buf,
                         size_t len, loff_t *offset)
{
    int bytes_read = 0;

    mutex_lock(&lab2_mutex);

    if (*offset >= buffer_len) {
        mutex_unlock(&lab2_mutex);
        return 0;
    }

    if (len > buffer_len - *offset)
        len = buffer_len - *offset;

    if (copy_to_user(buf, device_buffer + *offset, len)) {
        mutex_unlock(&lab2_mutex);
        pr_err("asgn1_driver: copy_to_user failed\n");
        return -EFAULT;
    }

    *offset += len;
    bytes_read = len;
    read_count++;

    mutex_unlock(&lab2_mutex);

    pr_info("asgn1_driver: sent %d bytes to user\n", bytes_read);
    return bytes_read;
}

static ssize_t lab2_write(struct file *file, const char __user *buf,
                          size_t len, loff_t *offset)
{
    char *temp_buffer;
    size_t data_len;

    mutex_lock(&lab2_mutex);

    if (read_only) {
        mutex_unlock(&lab2_mutex);
        return -EPERM;
    }

    if (len > BUFFER_SIZE - 1)
        len = BUFFER_SIZE - 1;

    temp_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!temp_buffer) {
        mutex_unlock(&lab2_mutex);
        return -ENOMEM;
    }

    if (copy_from_user(temp_buffer, buf, len)) {
        kfree(temp_buffer);
        mutex_unlock(&lab2_mutex);
        return -EFAULT;
    }

    temp_buffer[len] = '\0';

    if (mode == 0) {
        data_len = snprintf(device_buffer, BUFFER_SIZE,
                            "[KERNEL]%s", temp_buffer);
    } else {
        data_len = len;
        memcpy(device_buffer, temp_buffer, data_len);
        device_buffer[data_len] = '\0';
    }

    if (data_len >= BUFFER_SIZE)
        data_len = BUFFER_SIZE - 1;

    buffer_len = data_len;
    write_count++;
    last_write_size = len;

    kfree(temp_buffer);
    mutex_unlock(&lab2_mutex);

    pr_info("asgn1_driver: received %zu bytes\n", len);

    return len;
}

static long asgn1_ioctl(struct file *file,
                        unsigned int cmd,
                        unsigned long arg)
{
    struct asgn1_stats stats;
    int new_mode;
    char version[64] = "asgn1_driver v1.0";

    switch (cmd) {
    case ASGN1_RESET_BUFFER:
        mutex_lock(&lab2_mutex);

        memset(device_buffer, 0, BUFFER_SIZE);
        buffer_len = 0;
        write_count = 0;
        read_count = 0;

        mutex_unlock(&lab2_mutex);

        pr_info("asgn1_driver: ASGN1_RESET_BUFFER\n");
        break;

    case ASGN1_GET_STATS:
        mutex_lock(&lab2_mutex);

        stats.open_count = open_count;
        stats.write_count = write_count;
        stats.read_count = read_count;
        stats.buffer_len = buffer_len;
        stats.last_write_size = last_write_size;

        mutex_unlock(&lab2_mutex);

        if (copy_to_user((void __user *)arg,
                         &stats, sizeof(stats))) {
            return -EFAULT;
        }

        pr_info("asgn1_driver: ASGN1_GET_STATS\n");
        break;

    case ASGN1_SET_MODE:
        if (copy_from_user(&new_mode,
                           (int __user *)arg,
                           sizeof(new_mode))) {
            return -EFAULT;
        }

        if (new_mode != 0 && new_mode != 1)
            return -EINVAL;

        mutex_lock(&lab2_mutex);
        mode = new_mode;
        mutex_unlock(&lab2_mutex);

        pr_info("asgn1_driver: ASGN1_SET_MODE = %d\n",
                new_mode);
        break;

    case ASGN1_GET_VERSION:
        if (copy_to_user((void __user *)arg,
                         version, sizeof(version))) {
            return -EFAULT;
        }

        pr_info("asgn1_driver: ASGN1_GET_VERSION\n");
        break;

    default:
        pr_err("asgn1_driver: invalid ioctl command\n");
        return -ENOTTY;
    }

    return 0;
}


static struct file_operations lab2_fops = {
    .owner = THIS_MODULE,
    .open = lab2_open,
    .release = lab2_release,
    .read = lab2_read,
    .write = lab2_write,
    .unlocked_ioctl = asgn1_ioctl,
};

static int __init lab2_init(void) {
    int ret;

    pr_info("asgn1_driver: initializing module\n");

    /* Đăng ký character device với major number cố định */
    ret = register_chrdev(major_number, DEVICE_NAME, &lab2_fops);

    if (ret < 0) {
        pr_err("asgn1_driver: register_chrdev failed: %d\n", ret);
        return ret;
    }

    /* Tạo device class và device node tự động trong /sys */
    lab2_class = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(lab2_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(lab2_class);
    }

    lab2_device = device_create(lab2_class, NULL,
                                MKDEV(major_number, 0),
                                NULL, DEVICE_NAME);

    if (IS_ERR(lab2_device)) {
        class_destroy(lab2_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(lab2_device);
    }
/* Đăng ký sysfs attribute group */
    ret = sysfs_create_group(&lab2_device->kobj, &lab2_attr_group);
    if (ret) {
        pr_warn("asgn1_driver: sysfs_create_group failed\n");
    }


    pr_info("asgn1_driver: loaded, major=%d\n", major_number);
    /* Tạo /proc/lab2_info */
    proc_entry = proc_create("lab2_info", 0444, NULL, &lab2_proc_fops);
    if (!proc_entry)
        pr_warn("asgn1_driver: failed to create /proc/lab2_info\n");
    else
        pr_info("asgn1_driver: /proc/lab2_info created\n");

    return 0;
}

static void __exit lab2_exit(void) {
    if (proc_entry)
        proc_remove(proc_entry);
sysfs_remove_group(&lab2_device->kobj, &lab2_attr_group);
    device_destroy(lab2_class, MKDEV(major_number, 0));
    class_destroy(lab2_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    pr_info("asgn1_driver: module unloaded\n");
}

module_init(lab2_init);
module_exit(lab2_exit);
