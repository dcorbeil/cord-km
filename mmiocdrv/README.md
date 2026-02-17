# mmiocdrv

This very simple char driver controlling an GPIO0 31 on the Beaglebone Black. This driver exists for
academic only. It simply showcases how a gpio can be controlled from a character driver using
memory mapped IO.

## Loading

Inserting the module in kernel memory can simply be done by running

```shell
# File can either be the .ko or .ko.xz
insmod <path to kernel module>
```

Check for the driver's the major number and take note of it. This number is dynamic.

```shell
cat /proc/devices
```

Create the node to be able to communicate with the device driver.

```shell
mknod /dev/led c <major_number> 0
```

- `/dev/led`: the file we want to create that'll be associated with our char driver
- `c`: means char driver
- `0`: The minor number. Start at 0 because it's the first and only registered node for the driver

## Using

Turn on the led

```shell
echo 1 > /dev/led
```

Turn off the led

```shell
echo 0 > /dev/led
```

Query if it's on or off

```shell
cat /dev/led
```
