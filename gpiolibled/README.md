# gpiolibled

This very simple LED driver using GPIOlib that controls GPIO0 31 on the Beaglebone Black.
This driver exists for academic only. It simply showcases how a gpio can be controlled from the LED
framework driver using memory mapped IO.

GPIO lib uses a producer/consumer model for managing gpio. GPIO producers are something like controller
drivers and consumers can be an LED driver, keyboard driver or userspace application.

Some fiddling was needed to find the GPIO number at the time of writing this code. This is because
GPIO numbers are dynamic so now `gpio_request()` is deprecated

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

### Checking with libgpiod

If the host is equipped with `libgpiod`, it is possible to inspect our led and see some interesting
things after the driver was loaded.

```bash
$ gpioinfo

...

line  29:	"[rmii1_refclk]"	input
line  30:	"P9_11 [uart4_rxd]"	input
line  31:	"P9_13 [uart4_txd]"	output consumer="gpiolibled"
```

Note that our driver is a consumer
