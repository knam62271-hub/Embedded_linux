/*
 * File: sms_sensor_driver.c
 * Purpose: Sensor Monitoring System Character Device Driver
 * Author: Do Phuoc Hung - SE204688
 * Date: 06/11/2026
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/random.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/math64.h>

#define DEVICE_NAME "sms_sensor"
#define CLASS_NAME  "sms_sensor_class"

#define DEFAULT_INTERVAL_MS 100
#define MIN_INTERVAL_MS     100

#define SMS_MAGIC 'S'
#define SMS_SET_INTERVAL _IOW(SMS_MAGIC, 1, unsigned int)

/* Device information */
static int major_number;
static struct class *sms_class;
static struct device *sms_device;
static struct proc_dir_entry *sms_proc;

/* Shared data */
static DEFINE_MUTEX(sensor_mutex);

static unsigned int sample_interval_ms = DEFAULT_INTERVAL_MS;
static unsigned int read_count;
static int last_temperature;
static int last_humidity;
static u64 start_time_ms;
static u64 last_read_time_ms;

/* Module information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Do Phuoc Hung - SE204688");
MODULE_DESCRIPTION("Assignment 02 - Sensor Monitoring System Driver");
MODULE_VERSION("1.0");

/* Open device */
static int sms_open(struct inode *inode, struct file *file)
{
    return 0;
}

/* Close device */
static int sms_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* Read sensor data */
static ssize_t sms_read(struct file *file,
                        char __user *buf,
                        size_t count,
                        loff_t *ppos)
{
    char data[64];
    int temperature;
    int humidity;
    int len;
    unsigned int wait_ms;
    u64 now_ms;
    u64 next_read_ms;

    (void)file;
    (void)ppos;

    if (count == 0)
        return 0;

    mutex_lock(&sensor_mutex);

    /* Enforce minimum interval between successful reads */
    now_ms = div_u64(ktime_to_ns(ktime_get()), NSEC_PER_MSEC);

    if (last_read_time_ms != 0) {
        next_read_ms = last_read_time_ms + sample_interval_ms;

        while (now_ms < next_read_ms) {
            wait_ms = (unsigned int)(next_read_ms - now_ms);
            msleep(wait_ms);
            now_ms = div_u64(ktime_to_ns(ktime_get()), NSEC_PER_MSEC);
        }
    }

    /* Generate random sensor values */
    temperature = 15 + (get_random_u32() % 31);
    humidity = 30 + (get_random_u32() % 61);

    /* Get timestamp in milliseconds */
    now_ms = div_u64(ktime_to_ns(ktime_get()), NSEC_PER_MSEC);

    /* CSV format: timestamp_ms,temperature,humidity */
    len = scnprintf(data, sizeof(data),
                    "%llu,%d,%d\n",
                    (unsigned long long)now_ms,
                    temperature,
                    humidity);

    if (count < len) {
        mutex_unlock(&sensor_mutex);
        return -EINVAL;
    }

    if (copy_to_user(buf, data, len)) {
        mutex_unlock(&sensor_mutex);
        return -EFAULT;
    }

    /* Update statistics */
    read_count++;
    last_temperature = temperature;
    last_humidity = humidity;
    last_read_time_ms = now_ms;

    mutex_unlock(&sensor_mutex);

    return len;
}

/* Configure sampling interval using ioctl */
static long sms_ioctl(struct file *file,
                      unsigned int cmd,
                      unsigned long arg)
{
    unsigned int new_interval;

    (void)file;

    switch (cmd) {
    case SMS_SET_INTERVAL:

        if (copy_from_user(&new_interval,
                           (unsigned int __user *)arg,
                           sizeof(new_interval)))
            return -EFAULT;

        if (new_interval < MIN_INTERVAL_MS)
            return -EINVAL;

        mutex_lock(&sensor_mutex);
        sample_interval_ms = new_interval;
        mutex_unlock(&sensor_mutex);

        pr_info("sms_sensor: interval set to %u ms\n",
                new_interval);

        return 0;

    default:
        return -ENOTTY;
    }
}

/* File operations */
static const struct file_operations sms_fops = {
    .owner          = THIS_MODULE,
    .open           = sms_open,
    .release        = sms_release,
    .read           = sms_read,
    .unlocked_ioctl = sms_ioctl,
};

/* Display statistics in /proc/sms_stats */
static int sms_proc_show(struct seq_file *m, void *v)
{
    u64 uptime_ms;

    mutex_lock(&sensor_mutex);

    uptime_ms = div_u64(ktime_to_ns(ktime_get()), NSEC_PER_MSEC) - start_time_ms;

    seq_printf(m, "Sensor Monitoring System Statistics\n");
    seq_printf(m, "===================================\n");
    seq_printf(m, "read_count: %u\n", read_count);
    seq_printf(m, "last_temperature: %d\n", last_temperature);
    seq_printf(m, "last_humidity: %d\n", last_humidity);
    seq_printf(m, "uptime_seconds: %llu\n",
               (unsigned long long)div_u64(uptime_ms, 1000));

    mutex_unlock(&sensor_mutex);

    return 0;
}

static int sms_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, sms_proc_show, NULL);
}

static const struct proc_ops sms_proc_fops = {
    .proc_open    = sms_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* Initialize module */
static int __init sms_init(void)
{
    int ret;

    pr_info("sms_sensor: initializing driver\n");

    start_time_ms = div_u64(ktime_to_ns(ktime_get()), NSEC_PER_MSEC);

    /* Register character device with dynamic major number */
    major_number = register_chrdev(0, DEVICE_NAME, &sms_fops);

    if (major_number < 0) {
        pr_err("sms_sensor: register_chrdev failed\n");
        return major_number;
    }

    /* Create device class */
    sms_class = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(sms_class)) {
        ret = PTR_ERR(sms_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return ret;
    }

    /* Create /dev/sms_sensor */
    sms_device = device_create(sms_class, NULL,
                               MKDEV(major_number, 0),
                               NULL, DEVICE_NAME);

    if (IS_ERR(sms_device)) {
        ret = PTR_ERR(sms_device);
        class_destroy(sms_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return ret;
    }

    /* Create /proc/sms_stats */
    sms_proc = proc_create("sms_stats", 0444, NULL,
                           &sms_proc_fops);

    if (!sms_proc) {
        device_destroy(sms_class, MKDEV(major_number, 0));
        class_destroy(sms_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return -ENOMEM;
    }

    pr_info("sms_sensor: driver loaded, major=%d\n",
            major_number);

    return 0;
}

/* Remove module */
static void __exit sms_exit(void)
{
    if (sms_proc)
        proc_remove(sms_proc);

    if (sms_device)
        device_destroy(sms_class, MKDEV(major_number, 0));

    if (sms_class)
        class_destroy(sms_class);

    unregister_chrdev(major_number, DEVICE_NAME);

    pr_info("sms_sensor: driver unloaded\n");
}

module_init(sms_init);
module_exit(sms_exit);
