#include <linux/module.h>
/*
 * module_init(), module_exit()
 */
#include <linux/fs.h>
/*
 * register_chrdev_region()
 * struct file_operations
 */
#include <linux/cdev.h>
/*
 * cdev_init()
 * cdev_add()
 * cdev_del()
 */
#include <linux/kernel.h>
/*
 * printk()
 */
#include <linux/uaccess.h>
/*
 * copy_to_user()
 * copy_from_user()
 */
#include <linux/slab.h>
/*
 * kmalloc()
 */
//#include <linux/wait.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>

#include "scull.h"

struct scull_pipe {
	wait_queue_head_t inq, outq;
	char *buffer, *end;
	int buffersize;
	char *rp, *wp;
	int nreaders, nwriters;
//	struct fasync_struct *async_queue;
	struct mutex lock;
	struct cdev cdev;
};

static int scull_p_nr_devs = SCULL_P_NR_DEVS;
static int scull_p_buffer = SCULL_P_BUFFER;
static dev_t scull_p_devno;

static struct scull_pipe *scull_p_devices;

module_param(scull_p_nr_devs, int, 0);
module_param(scull_p_buffer, int, 0);





/**
 * Open and release
 */
static int scull_p_open(struct inode *inode, struct file *filp)
{
	struct scull_pipe *dev;
	
	dev = container_of(inode->i_cdev, struct scull_pipe, cdev);
	filp->private_data = dev;

	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

	if (!dev->buffer) {
		dev->buffer = kmalloc(scull_p_buffer, GFP_KERNEL);
		if(!dev->buffer) {
			mutex_unlock(&dev->lock);
			return -ENOMEM;
		}
	}
	dev->buffersize = scull_p_buffer;
	dev->end = dev->buffer + dev->buffersize;
	dev->rp = dev->wp = dev->buffer;

	if (filp->f_mode & FMODE_READ)
		dev->nreaders++;
	if (filp->f_mode & FMODE_WRITE)
		dev->nwriters++;
	mutex_unlock(&dev->lock);
	
	PDEBUG("buffer %pK\n", dev->buffer);

	return nonseekable_open(inode, filp);
	/* Ban lseek(), pread(), pwrite() */
}

//static int scull_p_fasync(int fd, struct file *filp, int mode)
//{
//	struct scull_pipe *dev = filp->private_data;
//
//	return fasync_helper(fd, filp, mode, &dev->async_queue);
//}

static int scull_p_release(struct inode *inode, struct file *filp)
{
//	struct scull_pipe *dev = filp->private_data;

//	scull_p_fasync(-1, filp, 0);
//	mutex_lock(&dev->lock);
//	if (filp->f_mode & FMODE_READ)
//		dev->nreaders --;
//	if (filp->f_mode & FMODE_WRITE)
//		dev->nwriters--;
//	if (dev->nreaders + dev->nwriters == 0) {
//		kfree(dev->buffer);
//		dev->buffer = NULL;
//	}
//	mutex_unlock(&dev->lock);
	return 0;
}

/**
 * Data management: read and write
 */
static ssize_t scull_p_read(struct file *filp, char __user *buf, size_t count,
		loff_t *f_pos)
{
	struct scull_pipe *dev = filp->private_data;

	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

	while(dev->rp == dev->wp) {
		mutex_unlock(&dev->lock);
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		PDEBUG("\"%s\" reading: going to sleep\n", current->comm);
		if (wait_event_interruptible(dev->inq, (dev->rp != dev->wp)))
			return -ERESTARTSYS;
		if (mutex_lock_interruptible(&dev->lock))
			return -ERESTARTSYS;
	}
	if (dev->wp > dev->rp)
		count = min(count, (size_t)(dev->wp - dev->rp));
	else
		count = min(count, (size_t)(dev->end - dev->rp));
	if (copy_to_user(buf, dev->rp, count)) {
		mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	dev->rp += count;
	if (dev->rp == dev->end)
		dev->rp = dev->buffer;
	mutex_unlock(&dev->lock);

	wake_up_interruptible(&dev->outq);
	PDEBUG("\"%s\" did read %lu bytes\n", current->comm, count);
	PDEBUG("buffer %pK wp %pK rp %pK\n", dev->buffer, dev->wp, dev->rp);
	return count;
}

static int spacefree(struct scull_pipe *dev)
{
	if (dev->rp == dev->wp)
		return dev->buffersize - 1;
	return ((dev->rp + dev->buffersize - dev->wp) % dev->buffersize) - 1;
}

static int scull_getwritespace(struct scull_pipe *dev, struct file *filp)
{
	while (spacefree(dev) == 0) {
		DEFINE_WAIT(wait);	// entry

		mutex_unlock(&dev->lock);
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		PDEBUG("\"%s\" writing: going to sleep", current->comm);
		prepare_to_wait(&dev->outq, &wait, TASK_INTERRUPTIBLE);
		if (spacefree(dev) == 0)
			/*
			 * Question: why "if" ?
			 * Answer: when mutex_unlock() runs out,
			 * a reader did read which will cause
			 * spacefree(dev) return nonzero. Then,
			 * the thread will sleep here until a
			 * new reader to wake up it.
			 */
			schedule();
		//if (signal_pending(current))
		//	return -ERESTARTSYS;
		if (mutex_lock_interruptible(&dev->lock))
			return -ERESTARTSYS;
	}
	return 0;
}

static ssize_t scull_p_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	struct scull_pipe *dev = filp->private_data;
	int result;

	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

	/* Make sure there's space to write */
	result = scull_getwritespace(dev, filp);
	if (result)
		return result;

	count = min(count, (size_t)(spacefree(dev)));
	if (dev->wp >= dev->rp)
		count = min(count, (size_t)(dev->end - dev->wp));
	else
		count = min(count, (size_t)(dev->rp - dev->wp -1));
	PDEBUG("Going to accept %lu bytes to %pK from %pK\n", count,
								dev->wp, buf);
	if (copy_from_user(dev->wp, buf, count)) {
		mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	dev->wp += count;
	if (dev->wp == dev->end)
		dev->wp = dev->buffer;
	mutex_unlock(&dev->lock);

	wake_up_interruptible(&dev->inq);

	PDEBUG("\"%s\" did write %lu bytes\n", current->comm, count);
	PDEBUG("buffer %pK wp %pK rp %pK\n", dev->buffer, dev->wp, dev->rp);
	return count;
}

/*
 * poll
 */

static unsigned int scull_p_poll(struct file *filp, poll_table *wait)
{
	struct scull_pipe *dev = filp->private_data;
	unsigned int mask = 0;

	poll_wait(filp, &dev->inq, wait);
	/*
	 * Add &dev->inq to wait;
	 */
	poll_wait(filp, &dev->outq, wait);
	if (dev->rp != dev->wp)
		mask |= POLLIN | POLLRDNORM;
	if (spacefree(dev))
		mask |= POLLOUT | POLLWRNORM;
	mutex_unlock(&dev->lock);
	return mask;
}

/*
 * The file operations for the pipe devcie
 * (some are overlayed with bare scull)
 */
struct file_operations scull_pipe_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = scull_ioctl,
	.open           = scull_p_open,
	.release        = scull_p_release,
	.read           = scull_p_read,
	.write          = scull_p_write,
	.poll           = scull_p_poll
};





/*
 * Set up a cdev entry
 */
static void scull_p_setup_cdev(struct scull_pipe *dev, int index)
{
	int err, devno = scull_p_devno + index;

	cdev_init(&dev->cdev, &scull_pipe_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding scullpipe%d", err, index);
}

/*
 * Initialize the pipe devs; return how many we did
 */
int scull_pipe_init(dev_t firstdev)
{
	int i, result;

	result = register_chrdev_region(firstdev, scull_p_nr_devs, "scullp");
	if (result < 0) {
		printk(KERN_NOTICE "Unable to gets scullp region, error %d\n",
								result);
		return 0;
	}
	scull_p_devno = firstdev;
	scull_p_devices = kmalloc(scull_p_nr_devs * sizeof(struct scull_pipe),
								GFP_KERNEL);
	if (scull_p_devices == NULL) {
		unregister_chrdev_region(firstdev, scull_p_nr_devs);
		return 0;
	}
	memset(scull_p_devices, 0, scull_p_nr_devs * sizeof(struct scull_pipe));
	for (i = 0; i < scull_p_nr_devs; i++) {
		// ?
		init_waitqueue_head(&(scull_p_devices[i].inq));
		init_waitqueue_head(&(scull_p_devices[i].outq));
		mutex_init(&scull_p_devices[i].lock);
		scull_p_setup_cdev(scull_p_devices + i, i);
	}
	return scull_p_nr_devs;
}

/*
 * This is called by cleanup_module or on failure.
 * It is required to never fail, even if nothing was initialized first.
 */
void scull_pipe_cleanup(void)
{
	int i;
	if (!scull_p_devices)
		return;
	for (i = 0; i < scull_p_nr_devs; i++) {
		cdev_del(&scull_p_devices[i].cdev);
		kfree(scull_p_devices[i].buffer);
	}
	kfree(scull_p_devices);
	unregister_chrdev_region(scull_p_devno, scull_p_nr_devs);
	scull_p_devices = NULL;
}
