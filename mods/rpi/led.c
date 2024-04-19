#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/ioport.h>

#include <asm/io.h>
#include "debug.h"
#include "gpio.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("NoCz");

//#define GPIO_FSEL0	GPIO_BASE + GPIO_FSEL0_OFFSET
//#define GPIO_SET0	GPIO_BASE + GPIO_SET0_OFFSET
//#define GPIO_CLR0	GPIO_BASE + GPIO_CLR0_OFFSET

static unsigned long gpio_phys = GPIO_BASE;
static void __iomem * gpio_base;	// GPIO Logical address
static int major, minor, nr = 1;
module_param(major, int, S_IRUGO);
module_param(minor, int, S_IRUGO);

struct led_dev {
	struct cdev cdev;
} led_device;

struct file_operations led_fops = {
	.owner	 = THIS_MODULE,
////	.open	 = led_open,
////	.release = led_release,
};

static void __exit led_cleanup(void)
{
	int val = 0;
	struct led_dev *dev = &led_device;

	printk(KERN_ALERT "LED clean up.\n");

	// GPIO operations

	/* Clean GPIO pins values */
	val |= (SET_GPCLR(2, 1) | SET_GPCLR(3, 1) | SET_GPCLR(4, 1));
	iowrite32(val, gpio_base + GPIO_CLR0_OFFSET);

	/* Clean and Del cdev */
	cdev_del(&dev->cdev);

	/* Unregister*/
	PDEBUG("Unregistered major %d, minor %d\n", major, minor);
	dev_t devno = MKDEV(major, minor);
	unregister_chrdev_region(devno, nr);

	/* Unremap I/O memory*/
	iounmap(gpio_base);
	PDEBUG("gpio_base %p released\n", gpio_base);

	/* Release I/O memory */
	release_mem_region(gpio_phys, GPIO_WIDTH);
	PDEBUG("Released the region 0x%lX\n", gpio_phys);

//	val = -1;
//	val &= ioread32(gpio_base + GPIO_CLR0_OFFSET);
//	PDEBUG("GPCLR0 0x%X\n", val);
	return;
}

static int __init led_init(void)
{
	int result, err;
	dev_t devno = 0;
	struct led_dev *dev = &led_device;

	/* Request and remap I/O memory */
	printk(KERN_ALERT "LED init\n");
	release_mem_region(gpio_phys, GPIO_WIDTH);
	if (!request_mem_region(gpio_phys, GPIO_WIDTH, "gpio")) {
		PDEBUG("Cant't get I/O memory address 0x%lX\n", gpio_phys);
		return -ENODEV;
	}

	PDEBUG("Got request a memory region 0x%lX\n", gpio_phys);

	gpio_base = ioremap(gpio_phys, GPIO_WIDTH);

	PDEBUG("Got gpio_base %p\n", gpio_base);

	/* Register device number */
	if (major) {
		devno = MKDEV(major, minor);
		register_chrdev_region(devno, nr, "led");
	} else {
		result = alloc_chrdev_region(&devno, minor, nr, "led");
		major = MAJOR(devno);
	}
	if (result < 0) {
		PDEBUG("Can't get major %d\n", major);
		return result;
	}
	PDEBUG("Registered at major %d, minor %d\n", major, minor);

	/* Init and Add cdev */
	cdev_init(&dev->cdev, &led_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err) {
		PDEBUG("Error %d adding cdev\n", err);
		goto fail;
	}

	// LED Operations
	int val = 0;

//	val = -1;
//	val &= ioread32(gpio_base + GPIO_FSEL0_OFFSET);
//	PDEBUG("GPFSEL0 0x%0X\n", val);

	// Set GPIO pins to out mode
	val |= (SET_GPFSEL(2, GPOUT) |
		SET_GPFSEL(3, GPOUT) |
		SET_GPFSEL(4, GPOUT));
	iowrite32(val, gpio_base + GPIO_FSEL0_OFFSET);

	// Set GPIO pins value
	val = ~(MASK_SET(2) | MASK_SET(3) | MASK_SET(4));

	val |= (SET_GPSET(2, 1) | SET_GPSET(3, 1) | SET_GPSET(4, 1));
	PDEBUG("GPSET0 0x%0X\n", val);
	iowrite32(val, gpio_base + GPIO_SET0_OFFSET);

//	val = -1;
//	val &= ioread32(gpio_base + GPIO_SET0_OFFSET);
//	PDEBUG("GPSET0 0x%0X\n", val);

	return 0;
fail:
	led_cleanup();
	return result;
}

module_init(led_init);
module_exit(led_cleanup);
