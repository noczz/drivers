#include <linux/module.h>
#include <linux/init.h>

#include <linux/sched.h> // struct current
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/types.h> // size_t #include <linux/completion.h>
#include <linux/fs.h>

#include <linux/completion.h> // !!!

MODULE_LICENSE("Dual BSD/GPL");

static int cmplt_major = 0;
static int cmplt_minor = 0;
static int cmplt_nr_devs = 1;

struct cdev cmplt_dev;

DECLARE_COMPLETION(comp);

//static int cmplt_open(struct inode *inode, struct file *filp)
//{
//	printk(KERN_DEBUG "complete open\n");
//	return 0;
//}
//
//static int cmplt_release(struct inode *inode, struct file *filp)
//{
//	printk(KERN_DEBUG "complete release\n");
//	return 0;
//}

ssize_t cmplt_read(struct file *filp, char __user *buf, size_t count,
		loff_t *pos)
{
	printk(KERN_DEBUG "process %i (%s) going to sleep\n",
			current->pid, current->comm);
	wait_for_completion(&comp);
	printk(KERN_DEBUG "awoken %i (%s)\n", current->pid, current->comm);

	return 0;
}

ssize_t cmplt_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *pos)
{
	printk(KERN_DEBUG "process %i (%s) awakening the readers...\n",
			current->pid, current->comm);
	complete(&comp);
	return count;
}

struct file_operations cmplt_fops = {
	.owner = THIS_MODULE,
	.read = cmplt_read,
	.write = cmplt_write,
//	.open = cmplt_open,
//	.release = cmplt_release,
};

int cmplt_init(void)
{
	int result, err, devno_;
	dev_t devno;

	if(cmplt_major) {
		devno = MKDEV(cmplt_major, cmplt_minor);
		result = register_chrdev_region(devno,
				cmplt_nr_devs, "complete");
	} else {
		result = alloc_chrdev_region(&devno, cmplt_minor,
				cmplt_nr_devs, "complete");
		cmplt_major = MAJOR(devno);
	}
	if(result < 0) {
		printk(KERN_WARNING "complete: can't get major %d\n",
				cmplt_major);
		return result;
	}

	if(cmplt_major == 0)
		cmplt_major = result;

	devno_ = MKDEV(cmplt_major, cmplt_minor);

	cdev_init(&cmplt_dev, &cmplt_fops);
	cmplt_dev.owner = THIS_MODULE;
	err = cdev_add(&cmplt_dev, devno_, 1);
	if(err)
		printk("error %d adding complete", err);

	printk(KERN_DEBUG "complete init\n");
	
	return 0;
}

void cmplt_cleanup(void)
{
	dev_t devno = MKDEV(cmplt_major, cmplt_minor);

	unregister_chrdev_region(devno, cmplt_nr_devs);
	
	printk(KERN_DEBUG "complete cleanup\n");

	return;
}

module_init(cmplt_init);
module_exit(cmplt_cleanup);
