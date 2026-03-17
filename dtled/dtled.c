#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define DRIVER_NAME "dtled"

#define LED_ON 1
#define LED_OFF 0

struct dtled_data_st {
    /* Descriptor for our gpio */
    struct gpio_desc *desc;
    struct led_classdev led_cdev;
};

static struct dtled_data_st* dtled_data;

static void dtled_setled(unsigned int status)
{
    if (status == LED_ON) {
        gpiod_set_value(dtled_data->desc, 1);
    } else {
        gpiod_set_value(dtled_data->desc, 0);
    }
}

static void dtled_change_state(struct led_classdev* led_cdev,
                                enum led_brightness brightness)
{
    if (brightness) {
        dtled_setled(LED_ON);
    } else {
        dtled_setled(LED_OFF);
    }
}

static int dtled_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct device_node *child = NULL;
    int result = 0;
    int gpio = 0;

    child = of_get_next_child(np, NULL);

    dtled_data = devm_kzalloc(&pdev->dev, sizeof(*dtled_data), GFP_KERNEL);

    if (!dtled_data) {
        return -ENOMEM;
    }

    gpio = of_get_named_gpio(child, "gpios", 0);

    result = devm_gpio_request(&pdev->dev, gpio, pdev->name);
    if (result) {
        dev_err(&pdev->dev, "Error requesting GPIO\n");
        return result;
    }

    dtled_data->desc = gpio_to_desc(gpio);

    dtled_data->led_cdev.name = of_get_property(child, "label", NULL);
    dtled_data->led_cdev.brightness_set = dtled_change_state;

    // Register the device gotten from the device tree in the led framework
    result = devm_led_classdev_register(&pdev->dev, &dtled_data->led_cdev);
    if (result) {
        dev_err(&pdev->dev, "Error registering led\n");
        return result;
    }

    gpiod_direction_output(dtled_data->desc, 0);

    dev_info(&pdev->dev, "initialized.\n");

    return 0;

}

static void dtled_remove(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "exiting.\n");
}

static const struct of_device_id of_dtled_match[] = {
    { .compatible = "cord,dtled" },
    {},
};

static struct platform_driver dtled_driver = {
    .driver = {
        .name = "dtled",
        .owner = THIS_MODULE,
        .of_match_table = of_dtled_match,
    },
    // Called when a device that matches this driver is registered in the platform bus
    // The thing that needs to match is the string set to .compatible. For example:
    // cordleds {
    //     compatible = "cord,dtled";
    //     led6 { ... }
    // };
    .probe = dtled_probe,
    // Called when a device that matches this driver is removed from the the platform bus
    .remove = dtled_remove
};

module_platform_driver(dtled_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple drive that controls a gpio through device tree");
MODULE_AUTHOR("David Corbeil <cord.sw.consulting@gmail.com>");
MODULE_VERSION("1.0");
