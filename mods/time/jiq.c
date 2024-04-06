#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/errno.h>
#include <linux/preempt.h>
#include <linux/workqueue.h>
MODULE_LICENSE("Dual BSD/GPL");

#ifdef DEBUG
#  ifdef __KERNEL__

#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "[ %s ] %s:%d %s() " \
		fmt, THIS_MODULE->name, __FILE__, __LINE__, __func__, ##args)

//#    define PDEBUG(fmt, args...) printk(KERN_DEBUG "[ scull ] "
//							fmt, ##args)
#  else
#    define PDEBUG(fmt, args...) printf("[ %s ] %s:%d %s() " \
		fmt, argv[0], __FILE__, __LINE__, __func__, ##args)
#  endif
#else
#  define PDEBUG(fmt, args...)
#endif

wait_queue_head_t jiq_wait;

static struct clientdata {
	struct work_struct jiq_work;
	struct seq_file *s;
	int len;
	unsigned long jiffies;
	long delay;
} jiq_data;

#define LIMIT (PAGE_SIZE-128)

static int jiq_print(struct clientdata *data)
{
	int len = data->len;
	struct seq_file *s = data->s;
	unsigned long j = jiffies;

	if (len > LIMIT) {
		wake_up_interruptible(&jiq_wait);
		return 0;
	}

	if (len == 0) {
		seq_puts(s, "  time\t\t delta\t preempt pid\t cpu\t command\n");
		len = s->count;
	} else {
		len = 0;
	}

	seq_printf(s, "  %ld\t %ld\t %d\t %d\t %d\t %s\n",
			j, j - data->jiffies, preempt_count(),
			current->pid, smp_processor_id(), current->comm);
	len += s->count;

	data->len += len;
	PDEBUG("len %d jiq_data.len %d\n", len, data->len);
	data->jiffies = j;
	return 1;
}

static void jiq_print_wq(struct work_struct *work)
{
	struct clientdata *data = container_of(work, struct clientdata,
								jiq_work);

	if (!jiq_print(data))
		return;

	schedule_work(&jiq_data.jiq_work);

	return;
}


static int jiq_read_wq_show(struct seq_file *s, void *v)
{
	wait_queue_entry_t wait;


	init_wait(&wait);
	jiq_data.len = 0;
	jiq_data.s = s;
	jiq_data.jiffies = jiffies;
	jiq_data.delay = 0;

	prepare_to_wait(&jiq_wait, &wait, TASK_INTERRUPTIBLE);

	schedule_work(&jiq_data.jiq_work); // jiq_print_wq()
	PDEBUG();
	schedule();
	finish_wait(&jiq_wait, &wait);
	
	return 0;
}

static int jiq_read_wq_open(struct inode *inode, struct file *file)
{
	PDEBUG();
	return single_open(file, jiq_read_wq_show, NULL);
}

static const struct proc_ops jiq_read_wq_pops = {
	.proc_open   = jiq_read_wq_open,
	.proc_read   = seq_read,
	.proc_lseek  = seq_lseek,
	.proc_release = single_release,
};

static int jiq_init(void)
{
	init_waitqueue_head(&jiq_wait);
	INIT_WORK(&jiq_data.jiq_work, jiq_print_wq);

	proc_create("jiqwq", 0, NULL, &jiq_read_wq_pops);

	return 0;
}

static void jiq_exit(void)
{
	remove_proc_entry("jiqwq", NULL);
	return;
}

module_init(jiq_init);
module_exit(jiq_exit);
