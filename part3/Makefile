KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD)/src modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/src clean
