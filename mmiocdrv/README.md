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

or observe that it's on or off by looking at it 😄

## Mapping of I/O memory

On in "real" scenarios you normally must "request" the physical memory region that backs a set
of device registers before mapping it into the kernel's virtual address space. The
commented-out call to `request_mem_region()` in the driver shows that step: it prevents
two drivers from claiming the same address range and helps avoid accidental conflicts.

On boards like the Beaglebone Black those GPIO registers are often already owned by the
system's GPIO driver, so a direct `request_mem_region()` will fail; that's why the
example leaves it commented out for educational purposes. The modern, safer pattern in
platform drivers is to use device-managed helpers such as `devm_platform_ioremap_resource()`
which both request and map the resource for you. Directly using `ioremap()` without
requesting the region is simpler for examples but may not work on real hardware and
isn't recommended for production drivers.

This can be observed on the actual board by running:

```bash
cat /proc/iomem
```

and observing that the memory region we're playing with is already controlled by the GPIO driver.

See [mmiocdrv.c](mmiocdrv.c#L132)