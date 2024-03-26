#ifndef COMMON_H
#define COMMON_H
#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#include<stdint.h>
#include<stdio.h>
#include<string.h>
//#define TESTPMM

//#ifdef TEST
//#include<stdlib.h>
//#endif
#define TESTKMT

#define IVTMAXSIZE (128)
#define IVTMIN (0)
#define IVTMAX (IVTMAXSIZE-1)

typedef struct interruptVector{
	int event;
	handler_t handler;
}IV;

struct interruptVectorTable{
    IV         *table;
};

struct interruptVectorTable* IVT;

struct CPU{
    struct task* currentTask;
    int ownedLockNum;
};//warning, assume that the cpu can only change itself!

struct CPU *cpu;

#endif
