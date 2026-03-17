# dtled

A Linux platform driver that controls a GPIO-backed LED registered with the kernel LED framework.
Unlike a driver that hardcodes a GPIO number, this one discovers the LED and its GPIO pin at boot
time by reading the device tree. The kernel matches the device tree node to this driver using the
`cord,dtled` compatible string, then calls `probe` to set everything up.

## Loading

Inserting the module in kernel memory can simply be done by running

```shell
# File can either be the .ko or .ko.xz
insmod <path to kernel module>
```

Once loaded, the kernel will match the device tree node to this driver and call `probe`. You can
confirm it worked with:

```shell
# Should show "dtled" in the list
lsmod | grep dtled

# The probe function logs this on success
dmesg | tail
```

## Unloading

When the module is removed, the `remove` callback fires and all `devm_*`-allocated resources
(GPIO, LED class device) are automatically freed by the kernel.

```shell
rmmod dtled
```

## Using

The sysfs path (`ipe:red:user`) comes from the `label` property of the device tree node. Any
non-zero brightness value turns the LED on; writing `0` turns it off.

Turn on the led

```shell
echo 1 > /sys/class/leds/ipe\:red\:user/brightness
```

Turn off the led

```shell
echo 0 > /sys/class/leds/ipe\:red\:user/brightness
```

Query if it's on or off

```shell
cat /sys/class/leds/ipe\:red\:user/brightness
```

or observe that it's on or off by looking at it 😄

### LED framework

With the LED framework, it is now possible to leverage its functionalities.

For example, **triggers**. A trigger is a kernel-side event source that drives the LED automatically
without any userspace involvement. The currently active trigger is shown in brackets.

```bash
# Looking at available triggers (active one is shown in [brackets])
cat /sys/class/leds/ipe\:red\:user/trigger

# Using the heartbeat trigger. This will make the LED act like a heartbeat that scales with CPU load
echo heartbeat > /sys/class/leds/ipe\:red\:user/trigger

# Another useful one: blink at a fixed interval (on/off delay in ms are tunable under the same dir)
echo timer > /sys/class/leds/ipe\:red\:user/trigger

# Go back to manual (brightness) control
echo none > /sys/class/leds/ipe\:red\:user/trigger
```

### Platform bus

The platform bus is a virtual bus used for devices that are not discoverable at runtime (i.e. not
PCI/USB). The device tree is the mechanism that describes these devices to the kernel.

We can look at all the devices on our platform bus by doing this

```shell
ls -al /sys/bus/platform/devices/
```

Under `devices/`, the name used is the name of the node given in the device tree

```dtb
cordleds { <---- this one here

    compatible = "cord,dtled";
    led6 {
        label = "ipe:red:user";  /* becomes the led_classdev name in sysfs */
        gpios = <&gpio 42 0>;    /* GPIO pin used to drive the LED */
    };
};
```

And then because of the `compatible`, it knows to match it to the `cord,dtled` driver name

```shell
ls -al /sys/bus/platform/drivers/dtled/cordleds/
```

Poking at the various symlinks in those directories, they all end up expressing the relationship
between driver and device. For instance, `drivers/dtled/cordleds` is a symlink back to the device
under `devices/`, and `devices/cordleds/driver` points back to the driver. This cross-linking is
how the kernel tracks the binding.
