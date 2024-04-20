#ifndef _SCULL_H_
#define _SCULL_H_

#define SCULLC_MAJOR   0
#define SCULLC_MINOR   0
#define SCULLC_NR_DEVS 3
#define SCULLC_QUANTUM 5
#define SCULLC_QSET    5

struct scullc_dev {
	void **data;
	struct scullc_dev *next;
	int quantum;
	int qset;
	size_t size;
	struct mutex lock;
	struct cdev cdev;
};

#endif
