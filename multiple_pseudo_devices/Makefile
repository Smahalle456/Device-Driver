obj-m := pcd_main.o

KERNEL_DIR=/lib/modules/4.15.0-132-generic/build/

all:
	make -C $(KERNEL_DIR)  M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR)  M=$(PWD) clean
