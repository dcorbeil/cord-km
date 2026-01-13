#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/types.h>

#define DRIVER_NAME "simplecdrv"

#define LED_ON 1
#define LED_OFF 0

static struct {
    dev_t devnum; // store major and minor number representing our driver. Is it vid:pid??
    struct cdev cdev; // store the char device that will be registered in the kernel
    unsigned int led_status;
} drvled_data;

static void drvled_setled(unsigned int status)
{
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

    // check if EOF
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
   const char *buffer,    /* The buffer to fill with data */
   size_t length,   /* The length of the buffer     */
   loff_t *offset)  /* Our offset in the file       */
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

static int drvled_open(struct inode *inode, struct file *file)
{
    pr_info("%s: Opening\n", DRIVER_NAME);

    return 0;
}

static int drvled_close(struct inode *inode, struct file *file)
{
    pr_info("%s: Closing\n", DRIVER_NAME);

    return 0;
}

static const struct file_operations drvled_fops = {
    .owner = THIS_MODULE,
    .write = drvled_write,
    .read = drvled_read,
    .open = drvled_open,
    .release = drvled_close,
};

/*
 * __init macro defines an init.text section for code for linker memory optimization linker
 * attributes. Similarly, data with the __initdata attribute goes into init.data. Once the init
 * function is invoked it will never be called again so once called, all the code and data in these
 * init sections are freed up via free_initmem()
*/
static int __init drvled_init(void)
{
    int result;

    // Allocate a range or char device numbers. Major is chosen dynamically and inserted in devnum (minor too?)
    result = alloc_chrdev_region(&drvled_data.devnum, 0, 1, DRIVER_NAME);
    if (result) {
        pr_err("%s: Failed to allocate driver number (%d)!\n", DRIVER_NAME, result);
        return result;
    }

    pr_debug("%s: Allocated major number: %d", DRIVER_NAME, drvled_data.devnum);

    // Initialize the char device structure. Tells the kernel that each time read()/write() is called
    // redirect to those callbacks to handle those operations. Like Zephyr, which makes sense as its
    // heavily inspired by the Linux kernel
    cdev_init(&drvled_data.cdev, &drvled_fops);

    // Register/add the allocated char device major and minor
    result = cdev_add(&drvled_data.cdev, drvled_data.devnum, 1);
    if (result) {
        pr_err("%s: Failed to register char device? (%d)\n", DRIVER_NAME, result);
        unregister_chrdev_region(drvled_data.devnum, 1);
        return result;
    }

    drvled_setled(LED_OFF);

    pr_info("%s: Initialized\n", DRIVER_NAME);
    return 0;
}

static void __exit drvled_exit(void)
{
    cdev_del(&drvled_data.cdev);
    unregister_chrdev_region(drvled_data.devnum, 1);
    pr_info("%s: exiting\n", DRIVER_NAME);
}

module_init(drvled_init);
module_exit(drvled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Corbeil <cord.sw.consulting@gmail.com>");
MODULE_VERSION("1.0");
