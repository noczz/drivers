static long delay = 1;
module_param(delay, long, 0);

static wait_queue_head_t jiq_wait;

static clientdata {
	struct work_struct jiq_work;
	struct delay_work jiq_delayed_work;
	struct timer_list jiq_timer;
	struct tasklet_struct jiq_tasklet;
	struct seq_file *s;
	long delay;
} jiq_data;

static int jiq_print
