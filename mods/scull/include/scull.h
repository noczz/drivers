#ifndef _SCULL_H_
#define _SCULL_H_

#undef PDEBUG

#ifdef SCULL_DEBUG
#  ifdef __KERNEL__

#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "[ %s ] %s:%d %s() " \
		fmt, THIS_MODULE->name, __FILE__, __LINE__, __func__, ##args)
//#    define PDEBUG(fmt, args...) printk(KERN_DEBUG
//							fmt, ##args)
#  else
#    define PDEBUG(fmt, args...) printf("[ %s ] %s:%d %s() " \
		fmt, argv[0], __FILE__, __LINE__, __func__, ##args)
#  endif
#else
#  define PDEBUG(fmt, args...)
#endif

// scull parameters

#define SCULL_MAJOR 0
#define SCULL_MINOR 0
#define SCULL_NR_DEVS 4
#define SCULL_QUANTUM 10
#define SCULL_QSET 5

#define SCULL_P_NR_DEVS 4
#define SCULL_P_BUFFER 5

/**
 * Structures
 */

// scull
extern int scull_quantum;
extern int scull_qset;

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
	struct mutex lock;
};

struct scull_adev_info {
	char *name;
	struct scull_dev *device;
	struct file_operations *fops;
};

/**
 * Prototypes
 */

// scull
int scull_trim(struct scull_dev *dev);

int scull_open(struct inode *inode, struct file *filp);
int scull_release(struct inode *inode, struct file *filp);
ssize_t scull_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos);
ssize_t scull_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos);
long scull_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

// access
int scull_access_init(dev_t dev);
void scull_access_cleanup(void);

// pipe
int scull_pipe_init(dev_t firstdev);
void scull_pipe_cleanup(void);

// proc(seq)
void scull_create_proc(void);
void scull_remove_proc(void);

// scull ioctl definitions
#define SCULL_IOC_MAGIC 'k'

#define SCULL_IOCRESET _IO(SCULL_IOC_MAGIC, 0)
/*
 * S - Set
 * T - Tell
 * G - Get
 * Q - Query
 * X - eXchange
 * H - sHift
 */
#define SCULL_IOCSQUANTUM	_IOW(SCULL_IOC_MAGIC, 1, int)
#define SCULL_IOCSQSET		_IOW(SCULL_IOC_MAGIC, 2, int)

#define SCULL_IOCTQUANTUM	_IO(SCULL_IOC_MAGIC, 3)
#define SCULL_IOCTQSET		_IO(SCULL_IOC_MAGIC, 4)

#define SCULL_IOCGQUANTUM	_IOR(SCULL_IOC_MAGIC, 5, int)
#define SCULL_IOCGQSET		_IOR(SCULL_IOC_MAGIC, 6, int)

#define SCULL_IOCQQUANTUM	_IO(SCULL_IOC_MAGIC, 7)
#define SCULL_IOCQQSET		_IO(SCULL_IOC_MAGIC, 8)

#define SCULL_IOCXQUANTUM	_IOWR(SCULL_IOC_MAGIC, 9, int)
#define SCULL_IOCXQSET		_IOWR(SCULL_IOC_MAGIC, 10, int)

#define SCULL_IOCHQUANTUM	_IO(SCULL_IOC_MAGIC, 11)
#define SCULL_IOCHQSET		_IO(SCULL_IOC_MAGIC, 12)

#define SCULL_IOC_MAXNR 14

#endif
