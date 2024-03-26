NAME           := kernel
SRCS           := framework/main.c $(shell find -L ./src/ -name "*.c")
INC_PATH       := include/ framework/

export AM_HOME := $(HOME)/Downloads/AM
ifeq ($(ARCH),)
export ARCH    := x86_64-qemu
endif

include $(AM_HOME)/Makefile

.PHONY : debug
debug :
	qemu-system-x86_64 -S -s -serial none -nographic build/$(NAME)-x86_64-qemu
###include ../Makefile.lab
###image: git

###NAME           := test

###test: git
###	@gcc $(shell find src/pmm.c)  \
###        $(shell find test/ -name "*.c") \
###        -Iframework -I../abstract-machine/klib/include -Itest -DTEST -lpthread -g\
###        -o build/test

###	@build/test 0

###testall: test
###	@build/test 0
###	@build/test 1
###	@build/test 2
