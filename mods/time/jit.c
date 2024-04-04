/*
 * jit.c -- the just-in-time module
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

/*
 * pde_data()
 */
#include <linux/proc_fs.h>

#include <linux/jiffies.h>

/*
 * init_waitqueue_head()
 */
#include <linux/wait.h>

/*
 * ktime_get_real_ts64()
 * ktime_get_coarse_real_ts64()
 */
#include <linux/ktime.h>

/*
 * seq_printf()
 * single_open()
 */
#include <linux/seq_file.h>

/*
 * struct tasklet_struct,
 * tasklet_schedule()
 * tasklet_hi_schedule()
 * tasklet_init()
 */
#include <linux/interrupt.h>

#undef PDEBUG

#ifdef SCULL_DEBUG
#  ifdef __KERNEL__

#    define PDEBUG(fmt, args...) printk( KERN_DEBUG \
		"%s:%d %s(): " fmt, __FILE__, __LINE__, __func__, ##args)

//#    define PDEBUG(fmt, args...) printk(KERN_DEBUG "[ scull ] "
//							fmt, ##args)
#  else
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ##args)
#  endif
#else
#  define PDEBUG(fmt, args...)
#endif

/*
 * This module is a silly one: it only embeds short code fragments
 * that show how time delays can be handled in the kernel.
 */

int delay = HZ; /* the default delay, expressed in jiffies */

module_param(delay, int, 0);

MODULE_AUTHOR("Alessandro Rubini");
MODULE_LICENSE("Dual BSD/GPL");

/* use these as data pointers, to implement four files in one function */
enum jit_files {
	JIT_BUSY,
	JIT_SCHED,
	JIT_QUEUE,
	JIT_SCHEDTO
};

/*
 * This function prints one line of data, after sleeping one second.
 * It can sleep in different ways, according to the data pointer
 */
static int condition = 0;

static int jit_fn_show(struct seq_file *s, void *v)
{
	unsigned long j0, j1, j2; /* jiffies */
	wait_queue_head_t wait;
	long data = (long)s->private;

	init_waitqueue_head(&wait);
	j0 = jiffies;
	j1 = j0 + delay;
	j2 = j0 + 2*delay;

	switch (data) {
		case JIT_BUSY:
			while (time_before(jiffies, j1))
				cpu_relax();
			break;
		case JIT_SCHED:
			while (time_before(jiffies, j1))
				schedule();
			break;
		case JIT_QUEUE:
			wait_event_interruptible_timeout(wait, condition, 2*delay);
			while (time_before(jiffies, j1))
				cpu_relax();
			condition = 1;
			break;
		case JIT_SCHEDTO:
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(delay);
			break;
	}
	j1 = jiffies; /* actual value after we delayed */

	seq_printf(s, "j0 %lu\t", j0);
	seq_printf(s, "j1 %lu\t", j1);
	seq_printf(s, "j1-j0 %lu\n", j1-j0);
	return 0;
}

static int jit_fn_open(struct inode *inode, struct file *file)
{
	PDEBUG("inode->i_private %ld\n", (long) inode->i_private);
	/*
	 * pde_data(inode) transmit inode->i_private to the s->private
	 * of jit_fn_show()
	 */
	return single_open(file, jit_fn_show, pde_data(inode));
}

static const struct proc_ops jit_fn_pops = {
	.proc_open	= jit_fn_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

/*
 * This file, on the other hand, returns the current time forever
 */
int jit_currentime_show(struct seq_file *s, void *v)
{
	unsigned long j1;
	u64 j2;

	j1 = jiffies;
	j2 = get_jiffies_64();

	struct timespec64 tv1;
	struct timespec64 tv2;
	ktime_get_real_ts64(&tv1);
	ktime_get_coarse_real_ts64(&tv2);
	seq_printf(s, "INITIAL_JIFFIES 0X%08lX\n", INITIAL_JIFFIES);
	seq_printf(s, "jiffies 0x%08lX jiffies64 0x%016llX\n"
		"tv1 %0i.%0i tv2 %0i.%0i\n",
	      j1, j2,
	      (int) tv1.tv_sec, (int) tv1.tv_nsec,
	      (int) tv2.tv_sec, (int) tv2.tv_nsec);

	return 0;
}

static int jit_currentime_open(struct inode *inode, struct file *file)
{
	return single_open(file, jit_currentime_show, NULL);
}

static const struct proc_ops jit_currentime_pops = {
	.proc_open	= jit_currentime_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

/*
 * timer
 */

int tdelay = 100;
module_param(tdelay, int, 0);

struct jit_data {
	struct timer_list timer;
	struct seq_file *s;
	wait_queue_head_t wait;
	unsigned long prevjiffies;
	int loops;
	// tasklet
	struct tasklet_struct tlet;
	int hi;
};

void jit_timer_fn(struct timer_list *t)
{
	/*
	 * /include/linux/timer.h (line 143)
	 *
	 * #define from_timer(var, callback_timer, timer_fieldname) \
	 * 	container_of(callback_timer, typeof(*var), timer_fieldname)
	 */
	struct jit_data *data = from_timer(data, t, timer);
	unsigned long j = jiffies;

	seq_printf(data->s, "  %lu \t%lu \t%d \t%d \t%d \t%s\n",
			j, j - data->prevjiffies, in_interrupt() ? 1 : 0,
			current->pid, smp_processor_id(), current->comm);

	if (--data->loops) {
		data->timer.expires += tdelay;
		data->prevjiffies = j;
		add_timer(&data->timer);
	} else {
		wake_up_interruptible(&data->wait);
	}
}

int jit_timer_show(struct seq_file *s, void *v)
{
	struct jit_data *data;
	unsigned long j =jiffies;

	data = kmalloc(sizeof(*data), GFP_KERNEL);
	if(!data)
		return -ENOMEM;

	init_waitqueue_head(&data->wait);

	seq_puts(s, "  time \t\tdelta \tinirq \tpid \tcpu \tcommand\n");
	seq_printf(s, "  %lu \t%lu \t%d \t%d \t%d \t%s\n",
			j, 0L, in_interrupt() ? 1 : 0,
			current->pid, smp_processor_id(), current->comm);

	/* fill the data for our timer funciton */
	data->prevjiffies = j;
	data->s = s;
	data->loops = 5;

	/* register the timer */
	timer_setup(&data->timer, jit_timer_fn, 0); // 0 means default timer
	data->timer.expires = j + tdelay;
	add_timer(&data->timer);

	wait_event_interruptible(data->wait, !data->loops);
	if (signal_pending(current))
		return -ERESTARTSYS;
	kfree(data);
	return 0;
}

static int jit_timer_open(struct inode *inode, struct file *file)
{
	return single_open(file, jit_timer_show, NULL);
}

static const struct proc_ops jit_timer_pops = {
	.proc_open = jit_timer_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

/*
 * tasklet
 */

void jit_tasklet_fn(unsigned long arg)
{
	struct jit_data *data = (struct jit_data *) arg;
	unsigned long j = jiffies;

	seq_printf(data->s, "  %lu \t%lu \t%d \t%d \t%d \t%s\n",
			j, j - data->prevjiffies, in_interrupt() ? 1 : 0,
			current->pid, smp_processor_id(), current->comm);

	if (--data->loops) {
		data->prevjiffies = j;
		if (data->hi)
			tasklet_hi_schedule(&data->tlet);
		else
			tasklet_schedule(&data->tlet);
	} else {
		wake_up_interruptible(&data->wait);
	}
}

int jit_tasklet_show(struct seq_file *s, void *v)
{
	struct jit_data *data;
	unsigned long j = jiffies;
	long hi = (long)s->private;

	data = kmalloc(sizeof(struct jit_data), GFP_KERNEL);
	if(!data)
		return -ENOMEM;

	init_waitqueue_head(&data->wait);

	seq_puts(s, "  time \t\tdelta \tinirq \tpid \tcpu \tcommand\n");
	seq_printf(s, "  %lu \t%lu \t%d \t%d \t%d \t%s\n",
			j, 0L, in_interrupt() ? 1 : 0,
			current->pid, smp_processor_id(), current->comm);

	/* fill the data for our tasklet function */
	data->prevjiffies = j;
	data->s = s;
	data->loops = 5;

	/* register the tasklet */
	/* data will be transmit to the arg of jit_tasklet_fn() */
	tasklet_init(&data->tlet, jit_tasklet_fn, (unsigned long)data);
	data->hi = hi;
	if (hi)
		tasklet_hi_schedule(&data->tlet);
	else
		tasklet_schedule(&data->tlet);

	wait_event_interruptible(data->wait, !data->loops);

	if (signal_pending(current))
		return -ERESTARTSYS;

	kfree(data);
	return 0;
}

static int jit_tasklet_open(struct inode *inode, struct file *file)
{
	return single_open(file, jit_tasklet_show, pde_data(inode));
}

static const struct proc_ops jit_tasklet_pops = {
	.proc_open    = jit_tasklet_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

int __init jit_init(void)
{
	proc_create("currentime", 0, NULL, &jit_currentime_pops);
	proc_create_data("jitbusy", 0, NULL, &jit_fn_pops, (void *)JIT_BUSY);
	proc_create_data("jitsched", 0, NULL, &jit_fn_pops, (void *)JIT_SCHED);
	proc_create_data("jitqueue", 0, NULL, &jit_fn_pops, (void *)JIT_QUEUE);
	proc_create_data("jitschedto", 0, NULL, &jit_fn_pops, (void *)JIT_SCHEDTO);
	/* JIT_BUSY will be stored at i_node->i_private */

	proc_create("jitimer", 0, NULL, &jit_timer_pops);
	proc_create("jitasklet", 0, NULL, &jit_tasklet_pops);
	proc_create_data("jitasklethi", 0, NULL, &jit_tasklet_pops, (void *)1);

	return 0; /* success */
}

void __exit jit_cleanup(void)
{
	remove_proc_entry("currentime", NULL);
	remove_proc_entry("jitbusy", NULL);
	remove_proc_entry("jitsched", NULL);
	remove_proc_entry("jitqueue", NULL);
	remove_proc_entry("jitschedto", NULL);

	remove_proc_entry("jitimer", NULL);
	remove_proc_entry("jitasklet", NULL);
	remove_proc_entry("jitasklethi", NULL);
}

module_init(jit_init);
module_exit(jit_cleanup);
