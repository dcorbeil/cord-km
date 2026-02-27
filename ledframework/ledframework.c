#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h> /* for ioremap */
#include <linux/ioport.h>
#include <linux/leds.h>
#include <linux/slab.h>

#define DRIVER_NAME "ledframework"

#define LED_ON 1
#define LED_OFF 0

/* Base address for GPIO0 control registers. Since this is hardcoded, it will only work for some
 * specific hardware. In this case I'm writing this driver for Beaglebone Black.
 * https://www.ti.com/lit/ug/spruh73q/spruh73q.pdf - Section 25.4
 */
#define GPIO0_BASE          0x44E07000
/* Pin capability register */
#define GPIO_OE             0x134
/* Pin state register */
#define GPIO_DATAOUT        0x13C
/* Clear pin register */
#define GPIO_CLEARDATAOUT   0x190

#define GPIO0_PAGE_SIZE 1024

/* Operate on GPIO0_31 */
#define GPIO_NUMBER 31
#define GPIO_BIT (1 << GPIO_NUMBER)

struct drvled_data_st {
    /* __iomem is a cookie for the sparse tool to communicate that the variable is io memory */
    void __iomem *regbase;
    struct led_classdev led_cdev;
};

static struct drvled_data_st* drvled_data;

static void drvled_setled(unsigned int status)
{
    /* Read the current register value */
    uint32_t val = readl(drvled_data->regbase + GPIO_DATAOUT);
    if (status == LED_ON) {
        val |= GPIO_BIT;
    } else {
        val &= ~GPIO_BIT;
    }
    /* Write the appropriate value to the register */
    writel(val, drvled_data->regbase + GPIO_DATAOUT);
}

static void drvled_setdirection(void)
{
    uint32_t val = readl(drvled_data->regbase + GPIO_OE);
    val &= ~GPIO_BIT;
    writel(val, drvled_data->regbase + GPIO_OE);
}

static void drvled_change_state(struct led_classdev* led_cdev,
                                enum led_brightness brightness)
{
    if (brightness) {
        drvled_setled(LED_ON);
    } else {
        drvled_setled(LED_OFF);
    }
}

/*
 * __init macro defines an init.text section for code for linker memory optimization linker
 * attributes. Similarly, data with the __initdata attribute goes into init.data. Once the init
 * function is invoked it will never be called again so once called, all the code and data in these
 * init sections are freed up via free_initmem()
*/
static int __init drvled_init(void)
{
    int result = 0;

    drvled_data = kzalloc(sizeof(*drvled_data), GFP_KERNEL);
    if (!drvled_data) {
        result = -ENOMEM;
        goto ret_err_kzalloc;
    }

    /* Map the requested memory region to virtual memory and store it so we can access it */
    drvled_data->regbase = ioremap(GPIO0_BASE, PAGE_SIZE);
    if (!drvled_data->regbase) {
        pr_err("%s: Error mapping I/O!\n", DRIVER_NAME);
        result = -ENOMEM;
        goto err_ioremap;
    }

    drvled_data->led_cdev.name = "ipe:red:user";
    /* Callback to be called when changing the brightness */
    drvled_data->led_cdev.brightness_set = drvled_change_state;

    /* Register the driver in the led framework */
    result = led_classdev_register(NULL, &drvled_data->led_cdev);
    if (result) {
        pr_err("%s: Error registering led\n", DRIVER_NAME);
        goto ret_err_led_classdev_register;
    }

    drvled_setdirection();

    drvled_setled(LED_OFF);

    pr_info("%s: Initialized\n", DRIVER_NAME);
    goto ret_ok;

ret_err_led_classdev_register:
    iounmap(drvled_data->regbase);
err_ioremap:
    kfree(drvled_data);
ret_err_kzalloc:
ret_ok:
    return result;
}

static void __exit drvled_exit(void)
{
    kfree(drvled_data);
    iounmap(drvled_data->regbase);
    pr_info("%s: exiting\n", DRIVER_NAME);
}

module_init(drvled_init);
module_exit(drvled_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple drive that controls GPIO0 31 using the LED framework");
MODULE_AUTHOR("David Corbeil <cord.sw.consulting@gmail.com>");
MODULE_VERSION("1.0");
