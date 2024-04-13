sperf : sperf.o
	gcc -o sperf sperf.o -lpthread
	rm -f sperf.o
sperf.o : sperf.c
	gcc -std=c99 -c sperf.c -o sperf.o -g
