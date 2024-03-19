NAME := amgame
SRCS := $(shell find -L ./src/ -name "*.c")
export MODULE  := L0
export AM_HOME := $(HOME)/downloads/AM
ifeq ($(ARCH),)
export ARCH := x86_64-qemu
endif

include $(AM_HOME)/Makefile

