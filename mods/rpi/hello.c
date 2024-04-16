#include <linux/module.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("Dual BSD/GPL");

int hello_init_module(void)
{
	printk(KERN_DEBUG "hello kernel!\n");
	return 0;
}

void hello_cleanup_module(void)
{
	printk(KERN_DEBUG "bye kernel.\n");
	return;
}

module_init(hello_init_module);
module_exit(hello_cleanup_module);
