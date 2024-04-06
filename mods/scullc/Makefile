# debug
DEBUG = y

ifeq ($(DEBUG),y)
	DEBFLAGS = -O -g -DDEBUG
else
	DEBFLAGS = -O2
endif

INCLUDES = -I $(PWD)/include

EXTRA_CFLAGS += $(DEBFLAGS)
EXTRA_CFLAGS += $(INCLUDES)

ifneq ($(KERNELRELEASE),)

scullc-objs := main.o

obj-m := scullc.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.mod modules.order *.symvers
