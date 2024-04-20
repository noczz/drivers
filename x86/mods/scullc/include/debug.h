#ifndef _DEBUG_H_
#define _DEBUG_H_

#undef PDEBUG

#ifdef DEBUG
#  ifdef __KERNEL__
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG \
		"[%s] %s:%d %s() " \
		fmt, THIS_MODULE->name, __FILE__, __LINE__, __func__, ##args)
#  else
#    define PDEBUG(fmt, args...) printf("[ %s ] %s:%d %s() " \
		fmt, argv[0], __FILE__, __LINE__, __func__, ##args)
#  endif
#else
#  define PDEBUG(fmt, args...)
#endif

#endif
