#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/slab.h>

#define DRIVER_NAME "gpiolibled"

#define LED_ON 1
#define LED_OFF 0

/* Operate on GPIO0_31 */
/* Although we set it to 639 because gpio numbers are dynamic. The gpio number for gpio0 31 was
 * found through: cat /sys/kernel/debug/gpio
*/
#define GPIO_NUMBER 639

struct drvled_data_st {
    /* Descriptor for our gpio */
    struct gpio_desc *desc;
    struct led_classdev led_cdev;
};

static struct drvled_data_st* drvled_data;

static void drvled_setled(unsigned int status)
{
    if (status == LED_ON) {
        gpiod_set_value(drvled_data->desc, 1);
    } else {
        gpiod_set_value(drvled_data->desc, 1);
    }
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

    result = gpio_request(GPIO_NUMBER, DRIVER_NAME);
    if (result) {
        pr_err("%s: Error requesting GPIO (%d)\n", DRIVER_NAME, result);
        goto ret_err_gpio_request;
    }

    /* Convert a gpio number to a descriptor */
    drvled_data->desc = gpio_to_desc(GPIO_NUMBER);

    drvled_data->led_cdev.name = "ipe:red:user";
    /* Callback to be called when changing the brightness */
    drvled_data->led_cdev.brightness_set = drvled_change_state;

    /* Register the driver in the led framework */
    result = led_classdev_register(NULL, &drvled_data->led_cdev);
    if (result) {
        pr_err("%s: Error registering led\n", DRIVER_NAME);
        goto ret_err_led_classdev_register;
    }

    /* Set the direction with initial value */
    gpiod_direction_output(drvled_data->desc, 0);

    pr_info("%s: Initialized\n", DRIVER_NAME);
    goto ret_ok;

ret_err_led_classdev_register:
    gpio_free(GPIO_NUMBER);
ret_err_gpio_request:
    kfree(drvled_data);
ret_err_kzalloc:
ret_ok:
    return result;
}

static void __exit drvled_exit(void)
{
    led_classdev_unregister(&drvled_data->led_cdev);
    gpio_free(GPIO_NUMBER);
    kfree(drvled_data);
    pr_info("%s: exiting\n", DRIVER_NAME);
}

module_init(drvled_init);
module_exit(drvled_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple drive that controls GPIO0 31 using GPIO lib");
MODULE_AUTHOR("David Corbeil <cord.sw.consulting@gmail.com>");
MODULE_VERSION("1.0");
