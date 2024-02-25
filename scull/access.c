#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h> // dev_t, struct file_operations,
#include <linux/cred.h> // current_uid(), current_euid(), current
#include <asm/atomic.h> // atomic_t, atomic_dec_and_test(), atomic_inc()
#include <linux/fs.h> 	// register_chrdev_region(), alloc_chrdev_region()
#include <linux/list.h> // LIST_HEAD, struct_list head
#include <linux/tty.h> // tty_devnum()

#include <linux/errno.h>

#include "scull.h"

static dev_t scull_a_firstdev;

// single-open

static struct scull_dev scull_s_device;
static atomic_t scull_s_availabe = ATOMIC_INIT(1);

static int scull_s_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev = &(scull_s_avilable);

	if (! atomic_dec_and_test(&scull_s_availabe)) {
		atomic_inc(&scull_s_availabe);
		return -EBUSY;
	}

	if ( (filp->f_flags & O_ACCMODE) == O_WRONLY)
		scull_trim(dev);

	filp->private_data = dev;
	return 0;
}

static int scull_s_release(struct inode *inode, struct file *filp)
{
	atomic_inc(&scull_s_available);
	return 0;
}

struct file_operations scull_sngl_fops = {
	.owner = THIS_MODULE,
	.read = scull_read,
	.write = scull_write,
	.unlocked_ioctl = scull_ioctl,
	.open = scull_s_open,
	.release = scull_s_release,
};

// multiple-open

static struct scull_dev scull_u_device;
static int scull_u_count;
static uid_t scull_u_owner;
static DEFINE_SPINLOCK(scull_u_lock);	// No header file referenced

static int scull_u_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev = &scull_u_device;

	spin_lock(&scull_u_lock);
	if (scull_u_count &&
			(scull_u_owner != current_uid().val) &&
			(scull_u_owner != current_euid().val) &&
			!capable(CAP_DAV_OVERRIDE)) {
		spin_unlock(&scull_u_lock);
		return -EBUSY;
	}

	if (scull_u_count == 0)
		scull_u_owner = current_uid().val;

	scull_u_count++;
	spin_unlock(&scull_u_lock);
	
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

// blocking-open

static struct scull_dev scull_w_device;
static int scull_w_count;
static uid_t scull_w_owner;
static DECLARE_WAIT_QUEUE_HEAD(scull_w_wait);
static DEFINE_SPINLOCK(scull_w_lock);

static inline scull_w_available(void)
{
	return scull_w_count == 0 ||
		scull_w_owner == current_uid().val ||
		scull_w_owner == current_euid().val ||
		capable(CAP_DAV_OVERRIDE);
}

static int scull_w_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev = &scull_w_device;
	
	spin_lock(&scull_write);
	while(! scull_w_available()) {
		spin_unlock(&scull_w_lock);
		if (filp->f_flags & O_NONBLOCK) return -EAGAIN;
		if (wait_event_interruptible(scull_w_wait, scull_w_available()))
			return -ERESTARTSYS;
		spin_lock(&scull_w_lock);
	}
	if (scull_w_count == 0)
		scull_w_owner = current_uid().val; // grab first user
	scull_w_count++;
	spin_unlock(&scull_w_lock);

	if( (filp->f_flags & O_ACCMODE) == O_WRONLY)
		scull_trim(dev);
	filp->private_data = dev;
	return 0;
}

static int scull_w_release(struct *inode, struct file *filp)
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
	.release        = scull_u_release,
};

// cloned private device

struct scull_listitem {
	struct scull_dev device;
	dev_t key;
	struct list_head list;
}

static LIST_HEAD(scull_c_list);
static DEFINE_SPINLOCK(scull_c_lock);

static struct scull_dev scull_c_device;

static struct scull_dev *scull_c_lookfor_device(dev_t key)
{
	struct scull_listitem *lptr;

	list_for_each_entry(lptr, &scull_c_list, list) {
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

	list_add(&lptr->list, &scull_c_list);

	return &(lptr->device);
}

static int scull_c_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev;
	dev_t key;

	if (!current->signal->tty) {
		PDEBUG("Process \"%s\" has no ctl tty\n", current->comm);
		return -EINVAL;		// comm: the name of current process
	}

	// get the device number of the tty
	key = tty_devnum(current->signal->tty);

	spin_lock(&scull_c_lock);
	dev = scull_c_lookfor_device(key);
	spin_unlock(&scull_c_lock);

	if (!dev)
		return -ENOMEN;

	if ( (filp->f_flags & O_ACCMODE) == O_WRONLY )
		scull_trim(dev);
	filp->private_data = dev;
	return 0;
}

static int scull_c_release(struct inode *inode, struct file *filp)
{
	return 0;
}

struct file_operations scull_c_fops = {
	.owner = THIS_MODULE,
	.read = scull_read,
	.write = scull_write,
	.unlocked_ioctl = scull_ioctl,
	.open = scull_c_open,
	.release = scull_c_release,
};

static struct scull_adev_info {
	char *name;
	struct scull_dev *device;
	struct file_operations *fops;
} scull_access_devs[] = {
	{"scullsingle", &scull_s_devcie, &scull_s_fops},
	{"sculluid", &scull_u_device, &scull_u_fops},
	{"scullwuid", &scull_w_device, &scull_w_fops},
	{"scullpriv", &scull_c_device, &scull_c_fops}
};

#define SCULL_N_ADEVS 4

static void scull_access_setup(dev_t devno, struct scull_adev_info *devinfo)
{
	struct scull_dev *dev = devinfo->device;
	int err;

	dev->quantum = scull_quantum;
	dev->qset = scull_qset;
	mutex_init(&dev->lock);

	cdev_init(&dev->cdev, devinfo->fops);
	kobject_set_name(&dev->cdev.kobj, devinfo->name); // ? kobject_put()
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_NOTICE "Error %d adding %s\n", err, devinfo->name);
		kobject_put(&dev->cdev.kobj);
	} else
		printk(KERN_NOTICE "%s registered at %x\n", devinfo->name, devno);
}

int scull_access_init(dev_t firstdev)
{
	int result, i;

	result = register_chrdev_region(firstdev, SCULL_N_ADEVS, "sculla");
	if (result < 0) {
		printk(KERN_WARNING "sculla: device number registration failed\n");
		return 0;
	}
	// where is alloc_chrdev_region() ?

	scull_a_firstdev = firstdev;

	for (i = 0; i < SCULL_N_ADEVS; i++)
		scull_access_setup(firstdev + i, scull_access_devs + i);

	return SCULL_N_ADEVS;
}

void scull_access_cleanup(void)
{
	struct scull_listitem *lptr, *next;
	int i;
	for (i = 0; i < SCULL_N_ADEVS; i++) {
		struct scull_dev *dev = scull_access_devs[i].device;
		cdev_del(&dev->cdev);
		scull_trim(scull_access_devs[i].device);
	}

	list_for_each_entry_safe(lptr, next, &scull_c_list, list) {
		list_del(&lptr->list);
		scull_trim(&(lptr->device));
		kfree(lptr);
	}

	unregister_chrdev_region(scull_a_firstdev, SCULL_N_ADEVS);
	return;
}