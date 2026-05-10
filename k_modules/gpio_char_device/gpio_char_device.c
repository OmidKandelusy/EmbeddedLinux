/*
 * gpio_char_dev.c - GPIO interrupt exposed as a misc character device
 *
 * Registers /dev/gpio_event. A user space process can block on read()
 * and will wake up each time the button triggers a rising-edge interrupt.
 * Each read returns a single byte containing the low 8 bits of the
 * cumulative press count.
 *
 * Hardware: BeagleBone Black, GPIO 526 (P9_12), active-high button.
 * 
 * Author: Omid Kandelusy
 */

// ==========================================================================================
// including the required header files

/** linux header files */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/atomic.h>
// ==========================================================================================
// variable, macro and constant definitions

#define DRIVER_NAME  "gpio_event"
#define BUTTON_GPIO  526

static int irq_number;

/* button press counter variable */
static atomic_t press_count = ATOMIC_INIT(0);

/* last count seen by a reader (used to detect a new event) */
static atomic_t last_seen = ATOMIC_INIT(0);

/* readers block here until an interrupt arrives */
static DECLARE_WAIT_QUEUE_HEAD(event_wq);
// ==========================================================================================
// callbacks and ISRs functions

static irqreturn_t button_isr(int irq, void *data)
{
    atomic_inc(&press_count);
    wake_up_interruptible(&event_wq);
    return IRQ_HANDLED;
}

// ==========================================================================================
// main functions:

/*
 * read() blocks until press_count differs from last_seen, then returns
 * one byte: the low 8 bits of the current press count.
 */
static ssize_t gpio_event_read(struct file *filp, char __user *buf,
                               size_t count, loff_t *ppos)
{
    int ret;
    u8  value;

    if (count == 0)
        return 0;

    /* sleep until a new press is available */
    ret = wait_event_interruptible(event_wq,
            atomic_read(&press_count) != atomic_read(&last_seen));
    if (ret)
        return -ERESTARTSYS;   /* signal interrupted the wait */

    /* snapshot and acknowledge the event */
    atomic_set(&last_seen, atomic_read(&press_count));
    value = (u8)(atomic_read(&press_count) & 0xFF);

    if (copy_to_user(buf, &value, 1))
        return -EFAULT;

    return 1;
}

static const struct file_operations gpio_event_fops = {
    .owner = THIS_MODULE,
    .read  = gpio_event_read,
};


/* Misc device descriptor */
static struct miscdevice gpio_event_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DRIVER_NAME,
    .fops  = &gpio_event_fops,
};

/** module init */
static int __init gpio_char_dev_init(void)
{
    int ret;

    /* 1. claim the GPIO */
    ret = gpio_request(BUTTON_GPIO, DRIVER_NAME);
    if (ret) {
        pr_err("%s: gpio_request(%d) failed: %d\n",
               DRIVER_NAME, BUTTON_GPIO, ret);
        return ret;
    }

    ret = gpio_direction_input(BUTTON_GPIO);
    if (ret) {
        pr_err("%s: gpio_direction_input failed: %d\n", DRIVER_NAME, ret);
        goto err_gpio;
    }

    /* 2. map GPIO → IRQ */
    irq_number = gpio_to_irq(BUTTON_GPIO);
    if (irq_number < 0) {
        pr_err("%s: gpio_to_irq failed: %d\n", DRIVER_NAME, irq_number);
        ret = irq_number;
        goto err_gpio;
    }

    /* 3. register interrupt handler — rising edge only */
    ret = request_irq(irq_number, button_isr,
                      IRQF_TRIGGER_RISING, DRIVER_NAME, NULL);
    if (ret) {
        pr_err("%s: request_irq failed: %d\n", DRIVER_NAME, ret);
        goto err_gpio;
    }

    /* 4. register the misc character device → /dev/gpio_event */
    ret = misc_register(&gpio_event_dev);
    if (ret) {
        pr_err("%s: misc_register failed: %d\n", DRIVER_NAME, ret);
        goto err_irq;
    }

    pr_info("%s: loaded — /dev/gpio_event ready (GPIO %d, IRQ %d)\n",
            DRIVER_NAME, BUTTON_GPIO, irq_number);
    return 0;

err_irq:
    free_irq(irq_number, NULL);
err_gpio:
    gpio_free(BUTTON_GPIO);
    return ret;
}

static void __exit gpio_char_dev_exit(void)
{
    misc_deregister(&gpio_event_dev);
    free_irq(irq_number, NULL);
    gpio_free(BUTTON_GPIO);
    pr_info("%s: unloaded\n", DRIVER_NAME);
}

module_init(gpio_char_dev_init);
module_exit(gpio_char_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BBB Portfolio");
MODULE_DESCRIPTION("GPIO interrupt exposed via /dev/gpio_event misc device");
MODULE_VERSION("1.0");