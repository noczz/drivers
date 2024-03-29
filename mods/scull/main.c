#include <linux/module.h> // module_init(), module_exit()
#include <linux/moduleparam.h> // module_param()
#include <linux/slab.h> // kmalloc()
#include <linux/fs.h> 	// register_chrdev_region(), alloc_chrdev_region()
#include <linux/cdev.h> // MKDEV(), struct inode, struct file_operations, dev_t
#include <linux/string.h> // memset()
// EFAULT, ENOMEM, ... path: /usr/include/asm-generic/errno-base.h
#include <linux/errno.h>
#include <linux/types.h> // size_t
#include <linux/fcntl.h> // O_ACCMODE
#include <linux/uaccess.h> // copy_to_user(), copy_from_user()

#include <linux/version.h> // LINUX_VERSION_CODE, KERNEL_VERSION()

#include "access_ok_version.h"
#include "scull.h"

MODULE_LICENSE("Dual BSD/GPL");


int scull_major = SCULL_MAJOR;
int scull_minor = SCULL_MINOR;
int scull_nr_devs = SCULL_NR_DEVS;
int scull_quantum = SCULL_QUANTUM;
int scull_qset = SCULL_QSET;

module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int, S_IRUGO);
module_param(scull_quantum, int, S_IRUGO);
module_param(scull_qset, int, S_IRUGO);
/*
 * S_IRUGO overwrite rights of sys/module/scull/parameters/scull*
 * R - Read, U - User, G - Group, O - Others
 */

struct scull_dev *scull_devices;

// scull Operations

struct file_operations scull_fops = {
	.owner	         = THIS_MODULE,
	.open	         = scull_open,
	.release         = scull_release,
	.read	         = scull_read,
	.write	         = scull_write,
	.unlocked_ioctl	 = scull_ioctl,
};

int scull_trim(struct scull_dev *dev)
{
	struct scull_qset *next, *dptr;
	int qset = dev->qset;
	int i;

	for (dptr = dev->data; dptr; dptr=next) {
		if (dptr->data) {
			for (i = 0; i < qset; i++)
				kfree(dptr->data[i]);
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next = dptr->next;
		kfree(dptr);
	}

	dev->size = 0;
	dev->quantum = scull_quantum;
	dev->qset = scull_qset;
	dev->data = NULL;
	return 0;
}

int scull_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev;

	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	filp->private_data = dev;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if (mutex_lock_interruptible(&dev->lock))
			return -ERESTARTSYS;
		scull_trim(dev);
		mutex_unlock(&dev->lock);
	}
	return 0;
}

int scull_release(struct inode *inode, struct file *filp)
{
	return 0;
}

struct scull_qset* scull_follow(struct scull_dev *dev, int item)
{
	struct scull_qset *dptr = dev->data; // dptr point to a quantum set
	if (!dptr) {
		dptr = dev->data = kmalloc(sizeof(struct scull_qset),
				GFP_KERNEL);
		if (dptr == NULL)
			return NULL;
		memset(dptr, 0, sizeof(struct scull_qset));
	}

	while(item--) {
		if (!dptr->next) {
			dptr->next = kmalloc(sizeof(struct scull_qset),
					GFP_KERNEL);
			if (dptr->next == NULL)
				return NULL;
			memset(dptr->next, 0, sizeof(scull_qset));
		}
		dptr = dptr->next;
		continue; // rest
	}

	return dptr;
}

ssize_t scull_read(struct file *filp, char __user *buf, size_t count,
		loff_t *f_pos)
{
	struct scull_dev *dev = filp->private_data; 
	struct scull_qset *dptr;

	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset;

	int item, s_pos, q_pos, rest;
	ssize_t retval = 0;

	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

	if (*f_pos >= dev->size)
		goto out;
	if (*f_pos + count > dev->size)
		count = dev->size - *f_pos;

	/*
	 * OFFSET
	 * ------
	 * 	scull_qset          - item
	 * 	scull_qset.data[]   - s_pos
	 * 	scull_qset.data[][] - q_pos
	 */
	rest = (long)*f_pos % itemsize;

	item = (long)*f_pos / itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;

	dptr = scull_follow(dev, item);

	if (dptr == NULL || !dptr->data || ! dptr->data[s_pos])
		goto out;

	if (count > quantum - q_pos)
		count = quantum - q_pos;

	PDEBUG("dev->size %lu f_pos %lld s_pos %d q_pos %d count %lu",
			dev->size, *f_pos, s_pos, q_pos , count);

	if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;

  out:
	mutex_unlock(&dev->lock);
	return retval;
}

ssize_t scull_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr;

	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset;

	int item, s_pos, q_pos, rest;

	ssize_t retval = -ENOMEM;

	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

	/*
	 * OFFSET
	 * ------
	 * 	scull_qset          - item
	 * 	scull_qset.data[]   - s_pos
	 * 	scull_qset.data[][] - q_pos
	 */
	rest = (long)*f_pos % itemsize;

	item = (long)*f_pos / itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;

	dptr = scull_follow(dev, item);
	if (dptr == NULL) {
		goto out;
	}
	if (!dptr->data) {	// Can we change the (char *) to (void *) ?
		dptr->data = kmalloc(qset * sizeof(void *), GFP_KERNEL);
		if (!dptr->data)
			goto out;
		memset(dptr->data, 0, qset * sizeof(void *));
	}
	if (!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		if (!dptr->data[s_pos])
			goto out;
	}

	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_from_user(dptr->data[s_pos]+q_pos, buf, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;

	if (dev->size < *f_pos)
		dev->size = *f_pos;

	PDEBUG("dev->size %lu item %d s_pos %d q_pos %d count %lu\n", 
			dev->size, item, s_pos, q_pos, count);
  out:
	mutex_unlock(&dev->lock);
	return retval;
}

long scull_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0, tmp;
	int retval = 0;

	if (_IOC_TYPE(cmd) != SCULL_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok_wrapper(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok_wrapper(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;

	switch(cmd) {
		case SCULL_IOCRESET:
			scull_quantum = SCULL_QUANTUM;
			scull_qset = SCULL_QSET;
			break;

		case SCULL_IOCSQUANTUM:
			if (!capable(CAP_SYS_ADMIN)) {
				return -EPERM;
			}
			retval = __get_user(scull_quantum, (int __user *)arg);
			break;

		case SCULL_IOCTQUANTUM:
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			scull_quantum = arg;
			break;

		case SCULL_IOCGQUANTUM:
			retval = __put_user(scull_quantum, (int __user *)arg);
			break;

		case SCULL_IOCQQUANTUM:
			return scull_quantum;

		case SCULL_IOCXQUANTUM:
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			tmp = scull_quantum;
			retval = __get_user(scull_quantum, (int __user *)arg);
			if (retval == 0)
				retval = __put_user(tmp, (int __user *)arg);
			break;

		case SCULL_IOCHQUANTUM:
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			tmp = scull_quantum;
			scull_quantum = arg;
			return tmp;

		case SCULL_IOCSQSET:
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			retval = __get_user(scull_qset, (int __user *)arg);
			break;

		case SCULL_IOCTQSET:
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			scull_qset = arg;
			break;

		case SCULL_IOCGQSET:
			retval = __put_user(scull_qset, (int __user *)arg);
			break;

		case SCULL_IOCQQSET:
			return scull_qset;

		case SCULL_IOCXQSET:
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			tmp = scull_qset;
			retval = __get_user(scull_qset, (int __user *)arg);
			if (retval == 0)
				retval = put_user(tmp, (int __user *)arg);
			break;

		case SCULL_IOCHQSET:
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			tmp = scull_qset;
			scull_qset = arg;
			return tmp;

		default:
			return -ENOTTY;
	}
	return retval;
}


// Init and Exit

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
	int err, devno = MKDEV(scull_major, scull_minor + index);

	cdev_init(&dev->cdev, &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding scull%d", err, index);
}

void scull_cleanup_module(void)
{
	int i;
	dev_t devno = MKDEV(scull_major, scull_minor);

	if (scull_devices) {
		for (i = 0; i < scull_nr_devs; i++) {
			scull_trim(scull_devices + i);
			cdev_del(&scull_devices[i].cdev);
		}
		kfree(scull_devices);
	}

#ifdef SCULL_DEBUG
	scull_remove_proc();
#endif

	unregister_chrdev_region(devno, scull_nr_devs);
	scull_access_cleanup();
	scull_pipe_cleanup();
}

int scull_init_module(void)
{
	int result, i;
	dev_t dev = 0;

	if (scull_major) {
		dev = MKDEV(scull_major, scull_minor);
		result = register_chrdev_region(dev, scull_nr_devs, "scull");
	} else {
		result = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs,
				"scull");
		scull_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "scull: can't get major %d\n",
				scull_major);
		return result;
	}

	scull_devices = kmalloc(scull_nr_devs * sizeof(struct scull_dev),
			GFP_KERNEL);
	if (!scull_devices) {
		result = -ENOMEM;
		goto fail;
	}
	memset(scull_devices, 0, scull_nr_devs * sizeof(struct scull_dev));

	for (i = 0; i < scull_nr_devs; i++) {
		scull_devices[i].quantum = scull_quantum;
		scull_devices[i].qset = scull_qset;
		mutex_init(&scull_devices[i].lock);
		scull_setup_cdev(&scull_devices[i], i);
	}

#ifdef SCULL_DEBUG
	scull_create_proc();
#endif

	dev += scull_nr_devs;
	dev += scull_access_init(dev);
	scull_pipe_init(dev);
	return 0;
fail:

	scull_cleanup_module();
	return result;
}

module_init(scull_init_module);
module_exit(scull_cleanup_module);
