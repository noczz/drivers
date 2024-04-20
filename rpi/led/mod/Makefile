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

# For Arm corss compiling add the line below
EXTRA_CFLAGS += -fno-stack-protector

ifneq ($(KERNELRELEASE),)

obj-m := led.o

else

PWD       := $(shell pwd)
KERNELDIR ?= ../../ker/linux-rpi-6.6.y

CROSS = ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

modules:
	make -C $(KERNELDIR) M=$(PWD) $(CROSS) modules
	make clean

endif

clean:
	rm -rf *.o *~ core .depend .*.cmd *.mod.c .tmp_versions *.mod modules.order *.symvers
