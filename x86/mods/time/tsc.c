#include <linux/init.h>
#include <linux/module.h>
#include <asm/msr.h>	// rdtscl(), rdtscll()
MODULE_LICENSE("Dual BSD/GPL");

static int hello_init(void)
{
	unsigned long long ini, end;
	ini = rdtsc();
	printk(KERN_ALERT "Hello, world\n");
	end = rdtsc();
	printk("time lapse: %lli\n", end - ini);
	return 0;
}

static void hello_exit(void)
{
	printk(KERN_ALERT "Goodbye, cruel world\n");
	return;
}

module_init(hello_init);
module_exit(hello_exit);
