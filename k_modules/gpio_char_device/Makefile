# linux kernel build system should be used to build the
# the object out of the source file.

KDIR ?= /lib/modules/$(shell uname -r)/build

obj-m := gpio_char_dev.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean