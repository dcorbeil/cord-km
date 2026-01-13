obj-m := simplecdrv.o

SRC := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

compress: all
	xz -fk simplecdrv.ko

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c *.ko.xz *.mod
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
