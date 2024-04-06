/*
 * scull cache
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/fs.h>

#include "scull.h"
#include "debug.h"

MODULE_LICENSE("Dual BSD/GPL");

int scullc_major   = SCULLC_MAJOR;
int scullc_minor   = SCULLC_MINOR;
int scullc_nr_devs = SCULLC_NR_DEVS;
int scullc_quantum = SCULLC_QUANTUM;
int scullc_qset    = SCULLC_QSET;

struct scullc_dev *scullc_devices;
struct kmem_cache *scullc_cache;

int scullc_trim(struct scullc_dev *dev)
{
	struct scullc_dev *next, *dptr;
	int qset = dev->qset;
	int i;
	
	for (dptr = dev; dptr; dptr = next) {
		if (dptr->data) {
			for (i = 0; i < qset; i++)
				if (dptr->data[i])
					kmem_cache_free(scullc_cache, dptr->data[i]);
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next = dptr->next;
		if (dptr != dev) kfree(dptr);
	}
	dev->size = 0;
	dev->qset = scullc_qset;
	dev->quantum = scullc_quantum;
	dev->next = NULL;
	return 0;
}

/*
 * open, release
 */

int scullc_open(struct inode *inode, struct file *filp)
{
	struct scullc_dev *dev;

	dev = container_of(inode->i_cdev, struct scullc_dev, cdev);

	if ( (filp->f_flags & O_ACCMODE) == O_WRONLY ) {
		if (mutex_lock_interruptible(&dev->lock))
			return -ERESTARTSYS;
		scullc_trim(dev);
		mutex_unlock(&dev->lock);
	}

	filp->private_data = dev;

	return 0;
}

int scullc_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/*
 * read, write, follow
 */
struct scullc_dev *scullc_follow(struct scullc_dev *dev, int item)
{
	while (item--) {
		if (!dev->next) {
			dev->next = kmalloc(sizeof(struct scullc_dev), GFP_KERNEL);
			memset(dev->next, 0, sizeof(struct scullc_dev));
		}
		dev = dev->next;
		continue;
	}
	return dev;
}

ssize_t scullc_read(struct file *filp, char __user *buf, size_t count,
		loff_t *f_pos)
{
	struct scullc_dev *dev = filp->private_data;
	struct scullc_dev *dptr;
	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	size_t retval = 0;

	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

	if (*f_pos > dev->size)
		goto fail;

	item = ((long) *f_pos) / itemsize;
	rest = ((long) *f_pos) % itemsize;
	s_pos = rest / itemsize; q_pos = rest % itemsize;

	dptr = scullc_follow(dev, item);

	if (!dptr->data)
		goto fail;
	if (!dptr->data[s_pos])
		goto fail;
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_to_user(buf, dptr->data[s_pos]+q_pos, count)) {
		retval = -EFAULT;
		goto fail;
	}
	mutex_unlock(&dev->lock);

	*f_pos += count;
	return count;

fail:
	mutex_unlock(&dev->lock);
	return retval;
}

ssize_t scullc_write(struct file *filp, const char __user *buf, size_t count,
		loff_t *f_pos)
{
	struct scullc_dev *dev = filp->private_data;
	struct scullc_dev *dptr;
	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum *qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = -ENOMEM;

	if (mutex_lock_interruptible (&dev->lock))
		return -ERESTARTSYS;

	item = ((long) *f_pos) / itemsize;
	rest = ((long) *f_pos) % itemsize;
	s_pos = rest / itemsize; q_pos = quantum % itemsize;

	dptr = scullc_follow(dev, item);
	if (!dptr->data) {
		dptr->data = kmalloc(qset * sizeof(void *), GFP_KERNEL);
		if (!dptr->data)
			goto fail;
		memset(dptr->data, 0, qset * sizeof(void *));
	}
	if (!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmem_cache_alloc(scullc_cache, GFP_KERNEL);
		if (!dptr->data[s_pos])
			goto fail;
		memset(dptr->data[s_pos], 0, quantum * sizeof(char));
	}
	if (count > quantum - q_pos)
		count = quantum - q_pos;
	if (copy_from_user(dptr->data[s_pos]+q_pos + q_pos, buf, count)) {
		retval = -EFAULT;
		goto fail;
	}
	*f_pos += count;

	if (dev->size < *f_pos)
		dev->size = *f_pos;
	mutex_unlock(&dev->lock);
	return count;

fail:
	mutex_unlock(&dev->lock);
	return retval;
}

/*
 * fops
 */
struct file_operations scullc_fops = {
	.owner   = THIS_MODULE,
	.open    = scullc_open,
	.release = scullc_release,
	.read    = scullc_read,
	.write   = scullc_write,
};

/*
 * set up
 */
static void scullc_setup_cdev(struct scullc_dev *dev, int index)
{
	int err, devno = MKDEV(scullc_major, index);

	cdev_init(&dev->cdev, &scullc_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);

	if (err)
		PDEBUG("Error %d adding scullc%d", err, index);
}

/*
 * scullc init and cleanup
 */
void scullc_cleanup(void)
{
	int i;
	dev_t devno = MKDEV(scullc_major, scullc_major);

	for (i = 0; i < scullc_nr_devs; i++) {
		cdev_del(&scullc_devices[i].cdev);
		scullc_trim(scullc_devices + i);
	}
	kfree(scullc_devices);

	if (scullc_cache)
		kmem_cache_destroy(scullc_cache);

	unregister_chrdev_region(devno, scullc_nr_devs);
}

int scullc_init(void)
{
	int result, i;
	dev_t devno = 0;

	/* scullc_major 0 use for selecting */
	if (scullc_major) {
		devno = MKDEV(scullc_major, scullc_minor);
		result = register_chrdev_region(devno, scullc_nr_devs, "scullc");
	} else {
		result = alloc_chrdev_region(&devno, scullc_minor, scullc_nr_devs,
								"scullc");
		scullc_major = MAJOR(devno);
	}
	if (result < 0) {
		PDEBUG( KERN_WARNING "can't get major %d\n", scullc_major);
		return result;
	}

	scullc_devices = kmalloc(scullc_nr_devs * sizeof (struct scullc_dev),
								GFP_KERNEL);
	if (!scullc_devices) {
		result = -ENOMEM;
		goto fail;
	}
	memset(scullc_devices, 0, scullc_nr_devs * sizeof(struct scullc_dev));
	for (i = 0; i < scullc_nr_devs; i++) {
		scullc_devices[i].quantum = scullc_quantum;
		scullc_devices[i].qset = scullc_qset;
		mutex_init(&scullc_devices[i].lock);
		scullc_setup_cdev(scullc_devices + i, i);
	}

	scullc_cache = kmem_cache_create("scullc", scullc_quantum, 0,
							SLAB_HWCACHE_ALIGN, NULL);
	if (!scullc_cache) {
		scullc_cleanup();
		return -ENOMEM;
	}

	return 0;

fail:
	unregister_chrdev_region(devno, scullc_nr_devs);
	return result;
}

module_init(scullc_init);
module_exit(scullc_cleanup);
