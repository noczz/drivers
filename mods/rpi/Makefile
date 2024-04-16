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

obj-m := hello.o

else

KERNELDIR ?= ker/linux
PWD       := $(shell pwd)
CROSS = ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $(CROSS) modules

endif

clean:
	rm -rf *.o *~ core .depend .*.cmd *.mod.c .tmp_versions *.mod modules.order *.symvers
