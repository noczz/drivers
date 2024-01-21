#ifndef _SCULL_H_
#define _SCULL_H_

#undef PDEBUG

//#define SCULL_DEBUG 1

//#ifdef SCULL_DEBUG
//#  ifdef __KERNEL__
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "[ scull ] ( file %s line %d func %s ) " fmt, __FILE__, __LINE__, __func__, ## args)
//#  else
//#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ##args)
//#  endif
//#else
//#  define PDEBUG(fmt, args...)
//#endif

struct scull_qset {
	void **data;
	struct scull_qset *next;
};

struct scull_dev {
	struct scull_qset *data;
	int quantum;
	int qset;
	unsigned long size;
	struct cdev cdev;
};

int scull_open(struct inode *inode, struct file *filp);
int scull_release(struct inode *inode, struct file *filp);
ssize_t scull_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos);
ssize_t scull_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos);

#endif
