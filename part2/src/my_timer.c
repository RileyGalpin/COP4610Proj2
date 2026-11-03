
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/time64.h>

#define PROC_NAME "timer"

static struct timespec64 previous_time;
static int has_previous_time = 0;

static ssize_t timer_read(struct file *file, char __user *user_buffer,
                          size_t count, loff_t *position)
{
    struct timespec64 current_time;
    struct timespec64 elapsed_time;
    char buffer[256];
    int length;

    ktime_get_real_ts64(&current_time);

    length = snprintf(buffer, sizeof(buffer),
                      "Current time: %lld.%09ld\n",
                      (long long)current_time.tv_sec,
                      current_time.tv_nsec);

    if (has_previous_time) {
        elapsed_time = timespec64_sub(current_time, previous_time);

        length += snprintf(buffer + length, sizeof(buffer) - length,
                           "Elapsed time: %lld.%09ld\n",
                           (long long)elapsed_time.tv_sec,
                           elapsed_time.tv_nsec);
    }

    previous_time = current_time;
    has_previous_time = 1;

    return simple_read_from_buffer(user_buffer, count, position,
                                    buffer, length);
}

static const struct proc_ops timer_ops = {
    .proc_read = timer_read,
};

static int __init timer_init(void)
{
    if (!proc_create(PROC_NAME, 0444, NULL, &timer_ops)) {
        printk(KERN_ERR "my_timer: failed to create /proc/timer\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "my_timer: /proc/timer created\n");
    return 0;
}

static void __exit timer_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "my_timer: /proc/timer removed\n");
}

module_init(timer_init);
module_exit(timer_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Timer Kernel Module");
