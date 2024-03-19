#include <common.h>
static void os_init()
{
	pmm->init();
}
#ifndef TEST
static void os_run()
{
	for (const char *s = "Hello World from CPU #*\n"; *s; s++)
	{
		putch(*s == '*' ? '0' + cpu_current() : *s);
	}
	void* p=pmm->alloc(5368);
	printf("%x\n",p);
	pmm->free(p);
	p=pmm->alloc(4096);
	printf("%x\n",p);
	pmm->free(p);
	p=pmm->alloc(5368);
	printf("%x\n",p);
	pmm->free(p);
	p=pmm->alloc(4);
	printf("%x\n",p);
	pmm->free(p);

	while(true);
}
#else
static void os_run(){
	for (const char *s = "Test from CPU #*\n"; *s; s++)
	{
		putch(*s == '*' ? '0' + cpu_current() : *s);
	}

	int testnum=200000;
	size_t size=0;
	void* p=NULL;
	void* q=NULL;
	void* r=NULL;
	printf("cpu_count:%d\n",cpu_count());
	printf("start\n");
	for(int i=1;i<=testnum;i++){
		size=rand();
		while(!size){
			size=rand();
		}
		p=pmm->alloc(size);
		size=rand()%8192;
		while(!size){
			size=rand()%8192;
		}
		r=pmm->alloc(size);
		size=rand()%4096;
		while(!size){
			size=rand()%4096;
		}
		q=pmm->alloc(size);
		pmm->free(p);
		if(!p||!q||!r){
			printf("stop at [%d]\n",i);
			break;
		}
	}
	printf("stop\n");
	halt(0);
	while(1);
	return;
}
#endif

MODULE_DEF(os) = {
	.init = os_init,
	.run = os_run,
};
