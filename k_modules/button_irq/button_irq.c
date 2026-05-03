#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/errno.h>

/*
 * Simple GPIO interrupt example for BeagleBone Black
 * - Hardcoded GPIO (for learning simplicity)
 * - Interrupt on rising edge (button press)
 */

#define BUTTON_GPIO  526

static unsigned int irq_number;

/* Interrupt handler */
static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    printk(KERN_INFO "[gpio_irq] Button interrupt triggered!\n");

    return IRQ_HANDLED;
}

/* Module init */
static int __init gpio_irq_init(void)
{
    int result = 0;

    printk(KERN_INFO "[gpio_irq] Initializing module\n");

    /* Request GPIO */
    if (!gpio_is_valid(BUTTON_GPIO)) {
        printk(KERN_ERR "[gpio_irq] Invalid GPIO\n");
        return -ENODEV;
    }

    result = gpio_request(BUTTON_GPIO, "sysfs_button_gpio");
    if (result) {
        printk(KERN_ERR "[gpio_irq] Failed to request GPIO\n");
        return result;
    }

    /* Set GPIO direction as input */
    result = gpio_direction_input(BUTTON_GPIO);
    if (result) {
        printk(KERN_ERR "[gpio_irq] Failed to set GPIO input\n");
        gpio_free(BUTTON_GPIO);
        return result;
    }

    /* Get IRQ number from GPIO */
    irq_number = gpio_to_irq(BUTTON_GPIO);
    printk(KERN_INFO "[gpio_irq] Mapped GPIO to IRQ: %d\n", irq_number);

    /* Request IRQ */
    result = request_irq(
        irq_number,
        gpio_irq_handler,
        IRQF_TRIGGER_FALLING,
        "gpio_irq_handler",
        NULL
    );

    if (result) {
        printk(KERN_ERR "[gpio_irq] Failed to request IRQ\n");
        gpio_free(BUTTON_GPIO);
        return result;
    }

    printk(KERN_INFO "[gpio_irq] Module loaded successfully\n");
    return 0;
}

/* Module exit */
static void __exit gpio_irq_exit(void)
{
    printk(KERN_INFO "[gpio_irq] Cleaning up module\n");

    free_irq(irq_number, NULL);
    gpio_free(BUTTON_GPIO);

    printk(KERN_INFO "[gpio_irq] Module unloaded\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Omid Kandelusy");
MODULE_DESCRIPTION("Simple button interrupt example for BeagleBone Black");
MODULE_VERSION("0.1");

module_init(gpio_irq_init);
module_exit(gpio_irq_exit);