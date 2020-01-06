DEBFLAGS = -O2
ccflags-y := -std=gnu11
EXTRA_CFLAGS = $(DEBFLAGS)
EXTRA_CFLAGS += -I$(LDDINC)

ifdef size
	EXTRA_CFLAGS += -DCEASAR_P_BUFFER=$(size)
endif

ifneq ($(KERNELRELEASE),)
#call from kernel build system

scull-objs := ceasar.o

obj-m := ceasar.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)




modules:
		$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD)/.../include modules
	
endif

debug:
		make DEBFLAGS=-DCEASAR_DEBUG

clean:
		rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

depend .depend dep:
		$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend, $(wildcard .depend))
include .depend
endif
