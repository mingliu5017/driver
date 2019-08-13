#obj-m += touch_msp430.o
#KDIR := /mnt/nfsroot/ming.liu/foxcnn/kernel-4.9/common    #如果是用于arm平台，则内核路径为arm内核的路径
#PWD = $(shell pwd)
#
#all:
#	make -C $(KDIR) M=$(PWD) modules
#
#clean:
#	rm -rf *.o
obj-m += touch_msp430.o
ARCH = arm64
CROSS_COMPILE = /opt/gcc-linaro-6.3.1-2017.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
cur_path =.
KDIR = ${cur_path}/../common
PWD = /mnt/nfsroot/ming.liu/belkin/kernel-4.9/touch_key

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	cp touch_msp430.ko ../../chrome/vendor/amlogic/s420/prebuilt/kernel/modules/

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) clean

