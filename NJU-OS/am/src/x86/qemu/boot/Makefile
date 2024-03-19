SRCS := start.S main.c
AM_HOME := $(HOME)/downloads/AM
bootblock.o: $(SRCS) Makefile
	@echo + CC $(SRCS)
	@$(CROSS_COMPILE)gcc -static -m32 -fno-pic -Os -nostdlib -Ttext 0x7c00 -I$(AM_HOME)/am/src -o bootblock.o $(SRCS)
	@python3 genboot.py bootblock.o

clean:
	rm -rf *.o
