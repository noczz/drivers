#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/ioport.h>

#include <asm/io.h>
#include "debug.h"
#include "gpio.h"

#define IOC_MAGIC 'k'
#define LED_IOCRESET	_IO(IOC_MAGIC, 0)
#define GPIO_ON		_IO(IOC_MAGIC, 1)
#define GPIO_OFF	_IO(IOC_MAGIC, 2)
#define IOC_MAXNR 2

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("NoCz");

static unsigned long gpio_phys = GPIO_BASE;
static void __iomem * gpio_base;	// GPIO Logical address
static int major, minor, nr = 1;
module_param(major, int, S_IRUGO);
module_param(minor, int, S_IRUGO);

struct led_dev {
	struct cdev cdev;
} led_device;

/*
 * LED Operations
 */

int led_open(struct inode *inode, struct file *filp)
{
	struct led_dev *dev;

	dev = container_of(inode->i_cdev, struct led_dev, cdev);
	filp->private_data = dev;

	return 0;
}

int led_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/*
 * GPIO registers operations
 */

void gpmode(unsigned long no, int mode)
{
	int val = -1;;

	val &= ioread32(gpio_base + GPIO_FSEL0_SHIFT);
	PDEBUG("GPFSEL0 0x%08X\n", val);

	val &= ~MASK_FSEL(no);
	val |= SET_GPFSEL(no, mode);
	PDEBUG("GPFSEL0 0x%08X\n", val);
	iowrite32(val, gpio_base + GPIO_FSEL0_SHIFT);
	return;
}

void gpset(unsigned long no)
{
	int val = 0;

	val |= SET_GPSET(no, 1);
	PDEBUG("GPSET0 0x%08X\n", val);
	iowrite32(val, gpio_base + GPIO_SET0_SHIFT);
	return;
}

void gpclr(unsigned long no)
{
	int val = 0;

	val |= SET_GPCLR(no, 1);
	PDEBUG("GPFCLR0 0x%08X\n", val);
	iowrite32(val, gpio_base + GPIO_CLR0_SHIFT);
	return;
}

long led_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int result = 0, err;
	if (_IOC_TYPE(cmd) != IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > IOC_MAXNR) return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = access_ok((void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_READ)
		err = access_ok((void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;

	switch(cmd) {
		case GPIO_ON:
			PDEBUG();
			gpmode(arg, GPOUT);
			gpset(arg);
			break;
		case GPIO_OFF:
			PDEBUG();
			gpclr(arg);
			break;
		default:
			return -ENOTTY;
	}
	return result;
}

struct file_operations led_fops = {
	.owner	 = THIS_MODULE,
	.open	 = led_open,
	.release = led_release,
	.unlocked_ioctl = led_ioctl,
};

static void __exit led_cleanup(void)
{
	struct led_dev *dev = &led_device;

	printk(KERN_ALERT "LED clean up.\n");

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

	return 0;
fail:
	led_cleanup();
	return result;
}

module_init(led_init);
module_exit(led_cleanup);
