#include <linux/version.h> // LINUX_VERSION_CODE, KERNEL_VERSION()
#include <linux/proc_fs.h> // proc_create()
#include <linux/seq_file.h>
/* struct seq_operations
 * seq_open(), ...
 */
#include <linux/fs.h> 	// register_chrdev_region(), alloc_chrdev_region()
#include <linux/cdev.h> // MKDEV(), struct inode, struct file_operations, dev_t

#include "scull.h"

extern int scull_a_nr_devs;
extern int scull_nr_devs;
extern struct scull_dev *scull_devices;
extern struct scull_adev_info scull_access_devs[];

loff_t seq_pos;

#ifdef SCULL_DEBUG

/**
 * seq start, next, stop, show...
 */

static void *scull_seq_start(struct seq_file *s, loff_t *pos)
{
	seq_pos = *pos;
	PDEBUG("seq_pos %lld\n", seq_pos);
	if (*pos >= scull_nr_devs + scull_a_nr_devs - 1)
		return NULL;
	else if (*pos >= scull_nr_devs)
		return scull_access_devs + *pos - scull_nr_devs;
	else
		return scull_devices + *pos;
}

static void *scull_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;
	seq_pos = *pos;
	PDEBUG("seq_pos %lld\n", seq_pos);
	if (*pos >= scull_nr_devs + scull_a_nr_devs -1)
		return NULL;
	else if (*pos >= scull_nr_devs)
		return scull_access_devs + *pos - scull_nr_devs;
	else
		return scull_devices + *pos;
}

static void scull_seq_stop(struct seq_file *s, void *v)
{
	// do nothing
	PDEBUG("seq_pos %lld\n", seq_pos);
}

static int scull_seq_show(struct seq_file *s, void *v)
{
	struct scull_dev *dev;
	int index;

	if (seq_pos >= scull_nr_devs) {
		dev = ((struct scull_adev_info *)v )->device;
		index = (int) ((struct scull_adev_info *)v - scull_access_devs);

		if (mutex_lock_interruptible(&dev->lock))
			return -ERESTARTSYS;

		// %i for decimal number
		seq_printf(s, "%s:\tqset %i \tquantum %i \tsize %li\n",
				scull_access_devs[index].name,
				dev->qset, dev->quantum, dev->size);
		mutex_unlock(&dev->lock);
	} else {
		dev = (struct scull_dev *) v;
		index = (int) (dev - scull_devices);

		if (mutex_lock_interruptible(&dev->lock))
			return -ERESTARTSYS;

		seq_printf(s, "scull%i:\tqset %i \tquantum %i \tsize %li\n",
				index, dev->qset, dev->quantum, dev->size);
		mutex_unlock(&dev->lock);
	}
		dev = (struct scull_dev *) v;

	return 0;
}

static struct seq_operations scull_seq_ops = {
	.start = scull_seq_start,
	.next  = scull_seq_next,
	.stop  = scull_seq_stop,
	.show  = scull_seq_show
};

static int scull_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &scull_seq_ops);
}

#  if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)

static const struct proc_ops scull_proc_ops = {
	.proc_open    = scull_proc_open,
	.proc_read    = seq_read,
	.proc_lseek  = seq_lseek,
	.proc_release = seq_release
};

#  else

static const struct file_operations scull_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = scull_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};

#  endif

void scull_create_proc(void)
{
	proc_create("scullseq", 0, NULL, &scull_proc_ops);
}

void scull_remove_proc(void)
{
	remove_proc_entry("scullseq", NULL);
}

#endif
