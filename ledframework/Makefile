obj-m := ledframework.o

SRC := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

compress: all
	xz -fk ledframework.ko

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c *.ko.xz *.mod .*.o
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
