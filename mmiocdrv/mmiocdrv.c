#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/io.h> /* for ioremap */
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/types.h>

#define DRIVER_NAME "mmiocdrv"

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

static struct {
    /* store major and minor number representing our driver */
    dev_t devnum;
    /* store the char device that will be registered in the kernel */
    struct cdev cdev;
    unsigned int led_status;
    /* __iomem is a cookie for the sparse tool to communicate that the variable is io memory */
    void __iomem *regbase;
} drvled_data;

static void drvled_setled(unsigned int status)
{
    /* Read the current register value */
    uint32_t val = readl(drvled_data.regbase + GPIO_DATAOUT);
    if (status == LED_ON) {
        val |= GPIO_BIT;
    } else {
        val &= ~GPIO_BIT;
    }
    /* Write the appropriate value to the register */
    writel(val, drvled_data.regbase + GPIO_DATAOUT);

    /* Track whether the led is on or off */
    drvled_data.led_status = status;
}

static ssize_t drvled_read(struct file *file,
   char *buffer,    /* The buffer to fill with data */
   size_t length,   /* The length of the buffer     */
   loff_t *offset)  /* Our offset in the file       */
{
    static const char * const msg[] = {"OFF\n", "ON\n"};
    /* The number of byte actually written to the buffer */
    int size;

    /* check if EOF */
    if (*offset > 0) {
        return 0;
    }

    size = strlen(msg[drvled_data.led_status]);
    if (size > length) {
        size = length;
    }

    /* The buffer is in the user data segment, not the kernel segment;
     * assignment won't work.  We have to use copy_to_user which copies data from
     * the kernel data segment to the user data segment. */
    if (copy_to_user(buffer, msg[drvled_data.led_status], size)) {
        return -EFAULT;
    }

    *offset += size;

    return size;
}

static ssize_t drvled_write(struct file *file,
   const char *buffer, /* The buffer to fill with data */
   size_t length,      /* The length of the buffer     */
   loff_t *offset)     /* Our offset in the file       */
{
    char kbuf = 0;

    /* Copy memory from the user data segment to the kernel data segment */
    if (copy_from_user(&kbuf, buffer, 1)) {
        return -EFAULT;
    }

    if (kbuf == '1') {
        drvled_setled(LED_ON);
        pr_info("LED_ON!\n");
    } else if (kbuf == '0') {
        drvled_setled(LED_OFF);
        pr_info("LED_OFF!\n");
    }

    return length;
}

static const struct file_operations drvled_fops = {
    .owner = THIS_MODULE,
    .write = drvled_write,
    .read = drvled_read,
    .open = NULL, /* Not setting this callback but leaving it for documentation purposes */
    .release = NULL, /* Not setting this callback but leaving it for documentation purposes */
};

static void drvled_setdirection(void)
{
    uint32_t val = readl(drvled_data.regbase + GPIO_OE);
    val &= ~GPIO_BIT;
    writel(val, drvled_data.regbase + GPIO_OE);
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

    /* Request to access memory region for the hardware registers. It will be denied and an error
     * will be returned if it's already been granted to another driver. This can be observed by
     * looking at /proc/iomem. At the address range we're trying to map (44e07000-44e07fff), we see
     * that it is taken by 44e07000.gpio gpio@0. Digging a bit deeper in the device tree, we can see
     * that ultimately omap4-gpio [0] is the driver controlling those gpios. If we go and look into
     * the drivers source code we can even see how it's controlling the same registers as we do. For
     * example GPIO_OE for direction [1]. We can even see the call to devm_platform_ioremap_resource()
     * that under the hood does both the request of memory and the remapping into virtual memory [2]
     * Note that this function is the new way of doing what we're doing here (or attempting to do)
     * as what we are doing is deprecated.
     *
     * [0]: https://github.com/torvalds/linux/blob/038d61fd642278bab63ee8ef722c50d10ab01e8f/arch/arm/boot/dts/ti/omap/am33xx-l4.dtsi#L160
     * [1]: https://github.com/torvalds/linux/blob/038d61fd642278bab63ee8ef722c50d10ab01e8f/drivers/gpio/gpio-omap.c#L1335
     * [2]: https://github.com/torvalds/linux/blob/038d61fd642278bab63ee8ef722c50d10ab01e8f/drivers/gpio/gpio-omap.c#L1446
     */
    /* Will leave the request and corresponding releases commented out for educational purposes.
     * Access is not granted on my platform. Normally that would be unacceptable but for academic
     * purposes, I judge it's ok.
     */
    // struct resource *res = request_mem_region(GPIO0_BASE, GPIO0_PAGE_SIZE, DRIVER_NAME);
    // if (!res) {
    //     pr_err("%s: Error requesting memory region", DRIVER_NAME);
    //     result = -EBUSY;
    //     goto ret_err_request_mem_region;
    // }

    /* Map the requested memory region to virtual memory and store it so we can access it */
    drvled_data.regbase = ioremap(GPIO0_BASE, PAGE_SIZE);
    if (!drvled_data.regbase) {
        pr_err("%s: Error mapping I/O!\n", DRIVER_NAME);
        result = -ENOMEM;
        goto err_ioremap;
    }

    /* Allocate a range or char device numbers. Major is chosen dynamically and inserted in devnum */
    result = alloc_chrdev_region(&drvled_data.devnum, 0, 1, DRIVER_NAME);
    if (result) {
        pr_err("%s: Failed to allocate driver number (%d)!\n", DRIVER_NAME, result);
        goto err_alloc_chrdev_region;
    }

    pr_debug("%s: Allocated major number: %d", DRIVER_NAME, drvled_data.devnum);

    /* Initialize the char device structure. Tells the kernel that each time read()/write() is called
     * redirect to those callbacks to handle those operations. Like Zephyr, which makes sense as its
     * heavily inspired by the Linux kernel
     */
    cdev_init(&drvled_data.cdev, &drvled_fops);

    /* Register/add the allocated char device major and minor */
    result = cdev_add(&drvled_data.cdev, drvled_data.devnum, 1);
    if (result) {
        pr_err("%s: Failed to register char device? (%d)\n", DRIVER_NAME, result);
        goto err_cdev_add;
    }

    drvled_setdirection();

    drvled_setled(LED_OFF);

    pr_info("%s: Initialized\n", DRIVER_NAME);
    goto ret_ok;

err_cdev_add:
    unregister_chrdev_region(drvled_data.devnum, 1);
err_alloc_chrdev_region:
    iounmap(drvled_data.regbase);
err_ioremap:
//     release_mem_region(GPIO0_BASE, GPIO0_PAGE_SIZE);
// ret_err_request_mem_region:
ret_ok:
    return result;
}

static void __exit drvled_exit(void)
{
    cdev_del(&drvled_data.cdev);
    unregister_chrdev_region(drvled_data.devnum, 1);
    iounmap(drvled_data.regbase);
    // release_mem_region(GPIO0_BASE, GPIO0_PAGE_SIZE);
    pr_info("%s: exiting\n", DRIVER_NAME);
}

module_init(drvled_init);
module_exit(drvled_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Extremely simple char driver that turns a hardcoded LED on/off on Beaglebone Black");
MODULE_AUTHOR("David Corbeil <cord.sw.consulting@gmail.com>");
MODULE_VERSION("1.0");
