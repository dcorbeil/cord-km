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

struct dtled_data {
    /* Descriptor for our gpio */
    struct gpio_desc *desc;
    struct led_classdev led_cdev;
    u8 can_sleep;
};

static void dtled_setled(struct led_classdev* led_cdev, unsigned int status)
{
    struct dtled_data* data = container_of(led_cdev, struct dtled_data, led_cdev);
    if (data->can_sleep) {
        gpiod_set_value_cansleep(data->desc, status);
    } else {
        gpiod_set_value(data->desc, status);
    }
}

static void dtled_change_state(struct led_classdev* led_cdev,
                                enum led_brightness brightness)
{
    if (brightness) {
        dtled_setled(led_cdev, LED_ON);
    } else {
        dtled_setled(led_cdev, LED_OFF);
    }
}

static int dtled_change_state_blocking(struct led_classdev *led_cdev,
    enum led_brightness value)
{
    dtled_setled(led_cdev, value);
    return 0;
}

struct gpio_leds_priv {
    int num_leds;
    struct dtled_data leds[] __counted_by(num_leds);
};

static int dtled_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    // struct device_node *child = NULL;
    int result = 0;
    int gpio = 0;
    int count = 0;
    int used = 0;

    struct device *dev = &pdev->dev;

	struct gpio_leds_priv *priv;

    count = device_get_child_node_count(dev);
    dev_info(&pdev->dev, "leds count: %d\n", count);

    // FIXME: Memory leak? memory is not deallocated or does it need to be deallocated??
    priv = devm_kzalloc(dev, struct_size(priv, leds, count), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;
    priv->num_leds = used;

    // TODO: Would the non-of variant be better (device_for_each_child_node_scoped)?
    // I don't fully know what the differences between the two are
    for_each_child_of_node_scoped(np, child) {

        struct dtled_data *dtled_dat = &priv->leds[used];
        struct gpio_led led = {};

        led.gpiod = devm_fwnode_gpiod_get(dev, of_fwnode_handle(child), NULL, GPIOD_ASIS, NULL);
        if (IS_ERR(led.gpiod)) {
            dev_err(&pdev->dev, "Failed to get GPIO '%pfw'\n", child);
            return -ENOENT;
        }

        dtled_dat->desc = led.gpiod;
        dtled_dat->led_cdev.name = of_get_property(child, "label", NULL);

        int can_sleep = gpiod_cansleep(dtled_dat->desc);
        dev_info(&pdev->dev, "can_sleep: %d\n", can_sleep);
        dtled_dat->can_sleep = can_sleep;
        int active_low = gpiod_is_active_low(dtled_dat->desc);
        dev_info(&pdev->dev, "is active low: %d\n", active_low);

        if (!dtled_dat->can_sleep) {
            dtled_dat->led_cdev.brightness_set = dtled_change_state;
        } else {
            // Some bus drivers can block while setting the led value. ie: gpio expanders
            // So we need to tell the platform driver so it can handle things correctly
            dtled_dat->led_cdev.brightness_set_blocking = dtled_change_state_blocking;
        }

        if (IS_ERR(dtled_dat->desc)) {
            dev_info(&pdev->dev, "Is error: Skipping unavailable LED gpio %d (%s)\n",
                    gpio, dtled_dat->led_cdev.name);
        }
        if (!dtled_dat->desc) {
            dev_info(&pdev->dev, "null: Skipping unavailable LED gpio %d (%s)\n",
                    gpio, dtled_dat->led_cdev.name);
        }

        dev_info(&pdev->dev, "name: %s\n", dtled_dat->led_cdev.name);
        dev_info(&pdev->dev, "desc: %p\n", dtled_dat->desc);

        // Register the device gotten from the device tree in the led framework
        result = devm_led_classdev_register(&pdev->dev, &dtled_dat->led_cdev);
        if (result) {
            dev_err(&pdev->dev, "Error registering led\n");
            return result;
        }

        gpiod_direction_output(dtled_dat->desc, 0);

        used++;
    }
    priv->num_leds = count;

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
