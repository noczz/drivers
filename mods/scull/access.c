#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cred.h> // current_uid(), current_euid(), current
#include <linux/fs.h> 	// register_chrdev_region(), alloc_chrdev_region()
#include <asm/atomic.h> // atomic_t, atomic_dec_and_test(), atomic_inc()
#include <linux/list.h> // LIST_HEAD, struct_list head
#include <linux/tty.h> // tty_devnum()

#include <linux/errno.h>

#include "scull.h"

#define ACCESS_NR_ADEVS 4

int scull_a_nr_devs = ACCESS_NR_ADEVS;

// single-open

static struct scull_dev scull_s_device;
static atomic_t scull_s_available = ATOMIC_INIT(1);

static int scull_s_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev;

	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	if (! atomic_dec_and_test(&scull_s_available)) {
		atomic_inc(&scull_s_available);
		return -EBUSY;
	}

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
		scull_trim(dev);

	filp->private_data = dev;
	return 0;
}

static int scull_s_release(struct inode *inode, struct file *filp)
{
	atomic_inc(&scull_s_available);
	return 0;
}

struct file_operations scull_s_fops = {
	.owner = THIS_MODULE,
	.read = scull_read,
	.write = scull_write,
	.unlocked_ioctl = scull_ioctl,
	.open = scull_s_open,
	.release = scull_s_release,
};





/*
 * scull_uid
 *
 * Discription:
 * 	This device can be opened by the rules below:
 * 		1. The numeber of users who have opened is 0
 * 		2. If the first condition is not met, the user
 * 		or effective user must be the owner. (The first
 * 		user will be record as a owner)
 * 		3. If the second condition is not met, the user
 * 		must have the DAC capablity
 */

static struct scull_dev scull_u_device;
static int scull_u_count = 0;	// initialize to 0 by default
static uid_t scull_u_owner = 0;	// initialize to 0(root) by default
static DEFINE_SPINLOCK(scull_u_lock);	// No header file referenced

static int scull_u_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev = &scull_u_device;

	spin_lock(&scull_u_lock);
	PDEBUG("USER(%u, %d) try to open scull_uid\n", current_uid().val,
							current_euid().val);
	PDEBUG("count %d, owner %u, cap %d\n",
		scull_u_count, scull_u_owner, capable(CAP_DAC_OVERRIDE));
	if (scull_u_count &&
			(scull_u_owner != current_uid().val) &&
			(scull_u_owner != current_euid().val) &&
			!capable(CAP_DAC_OVERRIDE)) {
			/**
			 *  CAP_DAC_OVERRIDE
			 *  	Bypass file read, write, and execute
			 *  	permission checks. (DAC is an abbreviation
			 *  	of "discretionary access control".)
			 */
			PDEBUG("USER(%u, %u) failed to open scull_uid\n",
				current_uid().val, current_euid().val);
		spin_unlock(&scull_u_lock);
		return -EBUSY;
	}

	if (scull_u_count == 0)
		scull_u_owner = current_uid().val;

	scull_u_count++;
	spin_unlock(&scull_u_lock);

	PDEBUG("USER(%u, %u) opened scull_uid\n",
		current_uid().val, current_euid().val);
	
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
		scull_trim(dev);
	filp->private_data = dev;
	return 0;
}

static int scull_u_release(struct inode *inode, struct file *filp)
{
	spin_lock(&scull_u_lock);
	scull_u_count--;
	spin_unlock(&scull_u_lock);
	return 0;
}

struct file_operations scull_u_fops = {
	.owner          = THIS_MODULE,
	.read           = scull_read,
	.write          = scull_write,
	.unlocked_ioctl = scull_ioctl,
	.open           = scull_u_open,
	.release        = scull_u_release,
};





/*
 * scull_wuid
 *
 * Discription:
 * 	The second user who don't have CAP_DAC_OVERRIDE, will
 * 	be blocked. When the first (or owner) closed, he will
 * 	be unblock. This device can be opened by the rules
 * 	below:
 * 		1. The numeber of users who have opened is 0
 * 		2. The user or effective user is the owner
 * 		(The first user will be record as a owner)
 * 		3. The user who have the DAC capablity
 */

static struct scull_dev scull_w_device;
static int scull_w_count;
static uid_t scull_w_owner;
static DECLARE_WAIT_QUEUE_HEAD(scull_w_wait);
static DEFINE_SPINLOCK(scull_w_lock);

static inline int scull_w_available(void)
{
	return scull_w_count == 0 ||
		scull_w_owner == current_uid().val ||
		scull_w_owner == current_euid().val ||
		capable(CAP_DAC_OVERRIDE);
}

static int scull_w_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev = &scull_w_device;
	
	PDEBUG("USER(%u, %d) try to open scull_wuid\n", current_uid().val,
							current_euid().val);
	PDEBUG("count %d, owner %u, cap %d\n",
		scull_u_count, scull_u_owner, capable(CAP_DAC_OVERRIDE));
	spin_lock(&scull_w_lock);
	while (! scull_w_available()) {
		spin_unlock(&scull_w_lock);
		if (filp->f_flags & O_NONBLOCK) return -EAGAIN;
		PDEBUG("USER(%u, %u) blocked\n",
			current_uid().val, current_euid().val);
		if (wait_event_interruptible(scull_w_wait, scull_w_available()))
			return -ERESTARTSYS;
		spin_lock(&scull_w_lock);
	}
	if (scull_w_count == 0)
		scull_w_owner = current_uid().val; // grab first user
	scull_w_count++;
	spin_unlock(&scull_w_lock);

	PDEBUG("USER(%u, %u) opened scull_wuid\n",
		current_uid().val, current_euid().val);

	if( (filp->f_flags & O_ACCMODE) == O_WRONLY)
		scull_trim(dev);
	filp->private_data = dev;
	return 0;
}

static int scull_w_release(struct inode *inode, struct file *filp)
{
	int temp;

	spin_lock(&scull_w_lock);
	scull_w_count--;
	temp = scull_w_count;
	spin_unlock(&scull_w_lock);

	if (temp == 0)
		wake_up_interruptible_sync(&scull_w_wait);
	return 0;
}

struct file_operations scull_w_fops = {
	.owner          = THIS_MODULE,
	.read           = scull_read,
	.write          = scull_write,
	.unlocked_ioctl = scull_ioctl,
	.open           = scull_w_open,
	.release        = scull_w_release,
};





/**
 * scull_priv
 *
 * Discription:
 * 	scull_priv is a private device for each tty.
 * 	Note that you should use command "tty" to check
 * 	which tty you have.
 */

static struct scull_dev scull_p_device;

struct scull_listitem {
	struct scull_dev device;
	dev_t key;
	struct list_head list;
};

static LIST_HEAD(scull_p_list);
static DEFINE_SPINLOCK(scull_p_lock);

static struct scull_dev *scull_p_lookfor_device(dev_t key)
{
	struct scull_listitem *lptr;

	// list  is the scull_listitem.list 's name
	list_for_each_entry(lptr, &scull_p_list, list) {
		if (lptr->key == key)
			return &(lptr->device);
	}

	lptr = kmalloc(sizeof(struct scull_listitem), GFP_KERNEL);
	if(!lptr)
		return NULL;

	memset(lptr, 0, sizeof(struct scull_listitem));
	lptr->key = key;
	scull_trim(&(lptr->device));
	mutex_init(&lptr->device.lock);

	list_add(&lptr->list, &scull_p_list);

	return &(lptr->device);
}

static int scull_p_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev;
	dev_t key;

	if (!current->signal->tty) {
		PDEBUG("Process \"%s\" has no ctl tty\n", current->comm);
		return -EINVAL;		// comm: the name of current process
	}

	// get the device number of the tty
	key = tty_devnum(current->signal->tty);

	spin_lock(&scull_p_lock);
	dev = scull_p_lookfor_device(key);
	spin_unlock(&scull_p_lock);

	if (!dev)
		return -ENOMEM;

	if ( (filp->f_flags & O_ACCMODE) == O_WRONLY )
		scull_trim(dev);
	filp->private_data = dev;
	return 0;
}

static int scull_p_release(struct inode *inode, struct file *filp)
{
	return 0;
}

struct file_operations scull_p_fops = {
	.owner = THIS_MODULE,
	.read = scull_read,
	.write = scull_write,
	.unlocked_ioctl = scull_ioctl,
	.open = scull_p_open,
	.release = scull_p_release,
};





/*
 * access init and clean up
 */

static dev_t scull_a_firstdev;

struct scull_adev_info scull_access_devs[] = {
	{"scull_single", &scull_s_device, &scull_s_fops},
	{"scull_uid", &scull_u_device, &scull_u_fops},
	{"scull_wuid", &scull_w_device, &scull_w_fops},
	{"scull_priv", &scull_p_device, &scull_p_fops}
};

static void scull_access_setup(dev_t devno, struct scull_adev_info *devinfo)
{
	struct scull_dev *dev = devinfo->device;
	int err;

	dev->quantum = scull_quantum;
	dev->qset = scull_qset;
	mutex_init(&dev->lock);

	cdev_init(&dev->cdev, devinfo->fops);
	kobject_set_name(&dev->cdev.kobj, devinfo->name);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err) {
		PDEBUG("error %d adding %s\n", err, devinfo->name);
		kobject_put(&dev->cdev.kobj);	// ? kobject_put()
	} else
		PDEBUG("%s registered at %X\n", devinfo->name, devno);
}

int scull_access_init(dev_t firstdev)
{
	int result, i;

	result = register_chrdev_region(firstdev, scull_a_nr_devs, "scullac");
	if (result < 0) {
		PDEBUG("device number registration failed\n");
		return 0;
	}
	// where is alloc_chrdev_region() ?

	scull_a_firstdev = firstdev;

	for (i = 0; i < scull_a_nr_devs; i++)
		scull_access_setup(firstdev + i, scull_access_devs + i);

	return scull_a_nr_devs;
}

void scull_access_cleanup(void)
{
	struct scull_listitem *lptr, *next;
	int i;
	for (i = 0; i < scull_a_nr_devs; i++) {
		struct scull_dev *dev = scull_access_devs[i].device;
		cdev_del(&dev->cdev);
		scull_trim(scull_access_devs[i].device);
	}

	list_for_each_entry_safe(lptr, next, &scull_p_list, list) {
		list_del(&lptr->list);
		scull_trim(&lptr->device);
		kfree(lptr);
	}

	unregister_chrdev_region(scull_a_firstdev, scull_a_nr_devs);
	return;
}
