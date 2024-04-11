#ifndef COMMON_H
#define COMMON_H
#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#include<stdint.h>
#include<stdio.h>
#include<string.h>
//#define TESTPMM


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
    struct task*    currentTask;
    bool            istate;
    int             ownedLockNum;
    Context         *spinCtx;
    struct CPU      *cpushot;
};//warning, assume that the cpu can only change itself!

struct CPU *cpu;

#define CURRENTCPU cpu[cpu_current()]

#define TESTKMT

#ifdef TESTKMT

typedef int printlock;

printlock pl;

extern void plinit();
extern void pllock();
extern void plunlock();

#define Print(...)\
    pllock();\
    printf(__VA_ARGS__);\
    plunlock();

#define CHECK(state)\
    ({Print("check%d\t%s:%d\n",state,__FILE__,__LINE__);})

#define PointerCheck(ptr)\
    if(!IN_RANGE(ptr,heap)){\
        Print("pointer error\t%s:%d\n",__FILE__,__LINE__);\
        halt(0);\
    }

#define HELP(expr,...)\
    if(expr){\
        pllock();\
        printf("file:%s\tline:%d\n",__FILE__,__LINE__);\
        printf(__VA_ARGS__);\
        plunlock();\
        halt(0);\
    }

#endif

//#define TESTPMM

#endif
