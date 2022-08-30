obj-m += driver.o
KVERSION = $(shell uname -r)
EXTRA_CFLAGS := -I"/usr/include/" -I"/usr/include/x86_64-linux-gnu/"

all:
		make -C /lib/modules/$(KVERSION)/build M=$(PWD)
clean:
		make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean