# ledframework

This very simple LED driver using the LED framework controls GPIO0 31 on the Beaglebone Black.
This driver exists for academic only. It simply showcases how a gpio can be controlled from the LED
framework driver using memory mapped IO.

Using the LED framework is one step closer to a real driver implementation as it leverages abstractions
provided by the Linux kernel. Upsides of using the LED framework compared to not using it is that it
does the creation of the led file on sysfs, has the concept of brightness, allows for using triggers
and more.

[v16.6 LED documentation](https://www.kernel.org/doc/html/v6.16/leds/leds-class.html)

## Loading

Inserting the module in kernel memory can simply be done by running

```shell
# File can either be the .ko or .ko.xz
insmod <path to kernel module>
```

## Using

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

For example, **triggers**

```bash
# Looking at available triggers
cat /sys/class/leds/ipe\:red\:user/trigger

# Using the heartbeat trigger. This will make the LED act like a heartbeat that scales with CPU load
echo heartbeat > /sys/class/leds/ipe\:red\:user/trigger
```
