#include <linux/module.h>
#include <linux/moduleparam.h>

#include "debug.h"

int dev_major, dev_minor;

module_param(scullc_major, int, S_IRUGO);
module_param(scullc_minor, int, S_IRUGO);

int scullc_init(void)
{
}

void unregister_cleanup(void)
{
	int i;
	if (!(dev_major || dev_minor))
		return;

	dev_t devno = MKDEV(dev_major, dev_minor);
	unregister_chrdev_region(devno, scullc_nr_devs);
}
