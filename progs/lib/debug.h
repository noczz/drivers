#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>

#ifndef PROG_NAME
	#define PROG_NAME arv[0]
#endif

#ifndef PROG_DEBUG
#	define DEBUG
#else
#	define DEBUG(fmt, args...)				\
		printf("[ " PROG_NAME " ] %s:%d %s(): " fmt,	\
			__FILE__, __LINE__, __func__, ##args)
#endif

#endif
