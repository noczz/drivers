#include <linux/module.h> // module_init(), module_exit()
#include <linux/slab.h> // kmalloc()
#include <linux/string.h> // memset()
#include <linux/cdev.h> // MKDEV(), struct inode, struct file_operations,
#include <linux/fs.h> // register_chrdev_region(), alloc_chrdev_region()
#include <linux/errno.h> // EFAULT, ENOMEM
#include <linux/types.h> // size_t
#include <linux/fcntl.h> // O_ACCMODE
#include <linux/uaccess.h> // copy_to_user(), copy_from_user()

#include <linux/seq_file.h>
/* struct seq_operations
 * seq_open(), ...
 */
#include <linux/proc_fs.h> // proc_create()

#include "scull.h"

int scull_major = 0;
int scull_minor = 0;
int scull_nr_devs = 4;
int scull_quantum = 10;
int scull_qset = 5;

struct scull_dev *scull_devices;

#ifdef SCULL_DEBUG

// seq_file Operations

static void *scull_seq_start(struct seq_file *s, loff_t *pos)
{
	if (*pos >= scull_nr_devs)
		return NULL;
	return scull_devices += *pos;
}

static void *scull_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;
	if(*pos >= scull_nr_devs)
		return NULL;
	return scull_devices += *pos;
}

static void scull_seq_stop(struct seq_file *s, void *v)
{
	// do nothing
}

static int scull_seq_show(struct seq_file *s, void *v)
{
	struct scull_dev *dev = (struct scull_dev *) v;
	struct scull_qset *d;
	int i;
	
	seq_printf(s, "\nDevice %i: qset %i, quantum %i, size %li\n", // %i for decimal number
		(int) (dev - scull_devices), dev->qset,
		dev->quantum, dev->size);
	for(d = dev->data; d; d = d->next) {
		seq_printf(s, "  item at %p, qset at %p\n", d, d->data);

	if (d->data && !d->next) /* dump only the last item */
		for (i = 0; i < dev->qset; i++) {
			if (d->data[i])
				seq_printf(s, "    %4i: %8p\n",
						i, d->data[i]);
		}
	}
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

static struct file_operations scull_proc_ops = {
	.owner = THIS_MODULE,
	.open = scull_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release
};

static void scull_create_proc(void)
{
	proc_create("scullseq", 0, NULL, &scull_proc_ops);
}

static void scull_remove_proc(void)
{
	remove_proc_entry("scullseq", NULL);
}

#endif


// scull Operations

struct file_operations scull_fops = {
	.owner =    THIS_MODULE,
	.open =     scull_open,
	.release =  scull_release,
	.read =     scull_read,
	.write =    scull_write,
};

int scull_trim(struct scull_dev *dev)
{
	struct scull_qset *next, *dptr;
	int qset = dev->qset;
	int i;

	for(dptr = dev->data; dptr; dptr=next) {
		if(dptr->data) {
			for(i = 0; i < qset; i++)
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

	if((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		scull_trim(dev);
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
	if(!dptr) {
		dptr = dev->data = kmalloc(sizeof(struct scull_qset),
				GFP_KERNEL);
		if(dptr == NULL)
			return NULL;
		memset(dptr, 0, sizeof(struct scull_qset));
	}

	while(item--) {
		if(!dptr->next) {
			dptr->next = kmalloc(sizeof(struct scull_qset),
					GFP_KERNEL);
			if(dptr->next == NULL)
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
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = 0;


	if (*f_pos >= dev->size)
		goto out;
	if (*f_pos + count > dev->size)
		count = dev->size - *f_pos;

	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum; q_pos = rest % quantum;

	dptr = scull_follow(dev, item);

	if (dptr == NULL || !dptr->data || ! dptr->data[s_pos])
		goto out;

	if (count > quantum - q_pos)
		count = quantum - q_pos;

	PDEBUG("dev->size %lu f_pos %lld\n", dev->size, *f_pos);
	PDEBUG("s_pos %d q_pos %d count %ld", s_pos, q_pos , count);

	if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;

  out:

	return retval;
}

ssize_t scull_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr;
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = -ENOMEM;

	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum; q_pos = rest % quantum;

	dptr = scull_follow(dev, item);
	if (dptr == NULL) {
		goto out;
	}
	if (!dptr->data) {
		dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
		if (!dptr->data)
			goto out;
		memset(dptr->data, 0, qset * sizeof(char *));
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

//	PDEBUG("item %d s_pos %d q_pos %d count %ld dev->size %ld", 
//			item, s_pos, q_pos, count, dev->size);
  out:
	return retval;
}


// Init and Exit

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
	int err, devno = MKDEV(scull_major, scull_minor + index);

	cdev_init(&dev->cdev, &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if(err)
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
}

int scull_init_module(void)
{
	int result, i;
	dev_t dev = 0;

	if(scull_major) {
		dev = MKDEV(scull_major, scull_minor);
		result = register_chrdev_region(dev, scull_nr_devs, "scull");
	} else {
		result = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs,
				"scull");
		scull_major = MAJOR(dev);
	}
	if(result < 0) {
		printk(KERN_WARNING "scull: can't get major %d\n",
				scull_major);
		return result;
	}

	scull_devices = kmalloc(scull_nr_devs * sizeof(struct scull_dev),
			GFP_KERNEL);
	if(!scull_devices) {
		result = -ENOMEM;
		goto fail;
	}
	memset(scull_devices, 0, scull_nr_devs * sizeof(struct scull_dev));

	for(i = 0; i < scull_nr_devs; i++) {
		scull_devices[i].quantum = scull_quantum;
		scull_devices[i].qset = scull_qset;
		scull_setup_cdev(&scull_devices[i], i);
	}

#ifdef SCULL_DEBUG
	scull_create_proc();
#endif

	return 0;
fail:
	scull_cleanup_module();
	return result;
}

module_init(scull_init_module);
module_exit(scull_cleanup_module);
