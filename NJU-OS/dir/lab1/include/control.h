#ifndef CONTROL_H
#define CONTROL_H
#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#include<stdint.h>

#define CPUMAXNUM (8)
#define PAGESIZE (4*1024)
#define PAGEEND(pageptr) (pageptr+PAGESIZE)
#define INITPAGENUM (10)

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define WRONG(expression)\
    ({if(!(expression)){\
        printf("WRONG\tfile:%s\tline:%d\n",__FILE__,__LINE__);\
        halt(0);\
    }\
    })
#define CHECK(state)\
    ({printf("check%d\t%s:%d\n",state,__FILE__,__LINE__);})

extern Area heap;

#define locked (1)
#define unlocked (0)
#define LOCKINIT (unlocked)
typedef int lock_t;

void spin_lock(lock_t* lock);

void spin_unlock(lock_t* lock);

typedef struct AllocatedNode{
    void* startptr;
    void* endptr;
    struct AllocatedNode* next;
}allocatednode_t,*allocatednode_ptr;
/*
pagenode manages the memery size of a page(4KiB)
*/
//assum that pagenode in cpumempool: !(combpage==0&&headnode.next==&tailnode)
typedef struct PageNode{
    int own;//0 server own, 1 cpu own
    int combpage;//the num of pages that should be considerd as a combinated part
    allocatednode_t headnode;
    allocatednode_t tailnode;
    struct PageNode* next;
}pagenode_t,*pagenode_ptr;
extern pagenode_ptr firstpagenode,lastpagenode;

typedef struct CPUMemPool{
    pagenode_ptr pages;
    lock_t cpumemlock;
}cpumempool_t,*cpumempool_ptr;
//cpu memery control block(at the top of the heap)
extern cpumempool_ptr pool;

typedef struct MemServer{
    pagenode_ptr pages;//pages that haven't been distributed
    lock_t serverlock;
}memserver_t,*memserver_ptr;
extern memserver_ptr server;

void allocatednode_init(allocatednode_ptr allocatednodeptr,void* startptr,int nodesize,void* next);

void pagenode_init(pagenode_ptr pagenodeptr,void* pageptr);
void* pagenode_delivery(pagenode_ptr pagenodeptr,size_t realsize);
int pagenode_receive(pagenode_ptr pagenodeptr,void* objptr);

void cpumempool_init(cpumempool_ptr cpumempoolptr);
void* cpumempool_delivery(cpumempool_ptr cpumempoolptr,size_t realsize);
int cpumempool_receive(cpumempool_ptr cpumempoolptr,void* objptr);//0 not found, 1 ok

void memserver_init(memserver_ptr memserver);
pagenode_ptr memserver_delivery(int combpage);
void memserver_receive(pagenode_ptr startpagenodeptr);

#ifdef TEST
int checkpagenodes(pagenode_ptr pages);
void checkblock(pagenode_ptr startptr,pagenode_ptr endptr);
#endif

#endif