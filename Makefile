# BINARY := fc1100

# OBJECTS := \
# 	chardev.o

# obj-m += driver.o
# KVERSION = $(shell uname -r)

# $(BINARY)-y := $(OBJECTS)

# all:
# 		make -C /lib/modules/$(KVERSION)/build M=$(PWD)
# clean:
# 		make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean

BINARY		:= fc1100
KERNEL		:= /lib/modules/$(shell uname -r)/build
ARCH		:= x86
C_FLAGS		:= -Wall
KMOD_DIR	:= $(shell pwd)
TARGET_PATH := /lib/modules/$(shell uname -r)/kernel/drivers/char

OBJECTS	:= \
	driver.o \
	chardev.o \

ccflags-y += $(C_FLAGS)

obj-m += $(BINARY).o

$(BINARY)-y := $(OBJECTS)

$(BINARY).ko:
	make -C $(KERNEL) M=$(KMOD_DIR) modules

install:
	cp $(BINARY).ko $(TARGET_PATH)
	depmod -a

uninstall:
	rm $(TARGET_PATH)/$(BINARY).ko
	depmod -a

clean:
	make -C $(KERNEL) M=$(KMOD_DIR) clean