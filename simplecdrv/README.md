# simplecdrv

This very simple char driver simulating controlling an LED by printing "LED_ON!"/"LED_OFF!" when
turned on/off through it's manually mapped node file location and returns 1 or 0 when the file is
read.

Collection of useful commands and notes

## Loading

Inserting the module in kernel memory can simply be done by running

```shell
# File can either be the .ko or .ko.xz
insmod <path to kernel module>
```

### Other method for loading module

Before loading, it might be needed to re-create the module dependency list if the module hasn't been
built at the same time as the OS. **I'm curious to know where and how that list is organized**

```shell
depmod
```

Put the module in `/lib/modules/$(uname -r)/updates`. **I've seen some kernel modules in `extras` too, I wonder what the difference is?**
**I also wonder if I could just put it in ``/lib/modules/$(uname -r)`

Loading the module:

```shell
modprobe simplecdrv
```

---

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
