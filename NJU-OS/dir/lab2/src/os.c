#include <os.h>



static void os_init()
{
	pmm->init();
	kmt->init();

}

static void os_run()
{
	iset(false);
#ifdef TESTKMT
	char words[30]="task1 created by cpu#\n";
	words[20]=cpu_current()+'0';
	HELP(kmt->create(pmm->alloc(sizeof(task_t)),"task1",task1,words)==1,"error!\n");
#endif
	iset(true);
	while(1){
		yield();
	}
#ifndef TESTPMM
	size_t size=0;
	while(!size||size<sizeof(int))size=rand()%16*1024;
	void *p=pmm->alloc(size);
	void *q=pmm->alloc(size);
	void *r=pmm->alloc(size);
	while(p&&q&&r){
		Print("size:%d,%p,%p,%p\n",size,p,q,r);
		*(int*)p=1;
		*(int*)p=1;
		*(int*)p=1;
		pmm->free(p);
		size=0;
		while(!size||size<sizeof(int))size=rand()%16*1024;
		p=pmm->alloc(size);
		q=pmm->alloc(size);
		r=pmm->alloc(size);
	}
	Print("stop\n");
	while(1);
#endif
	panic_on(1,"os_run return!\n");
}

static Context* os_trap(Event ev,Context *context)
{
	if(ev.event==EVENT_ERROR){
		if((void*)ev.ref==NULL&&(void*)ev.cause==NULL){Print("yes, empty\n");}
		Print("Unhandle signal '%s', badaddr = %p, cause = %p\n",
        ev.msg, (void*)ev.ref, (void*)ev.cause);
	}
	//HELP(ev.event==EVENT_YIELD||ev.event==EVENT_SYSCALL||ev.event==EVENT_PAGEFAULT||ev.event==EVENT_ERROR,"event error:%d\n",ev.event);
	HELP(ev.event==EVENT_ERROR,"event error:%d\n",ev.event);
	Context *next=NULL;
	Context *ctx=NULL;
	for(int i=0;i<IVTMAXSIZE;i++){
		if((IVT->table)[i].event==EVENT_NULL||(IVT->table)[i].event==ev.event){
			ctx=(IVT->table)[i].handler(ev,context);
			if(ctx){
				next=ctx;
				break;
			}
		}
	}
	panic_on(next==NULL,"ret error!\n");
	assert(next->rip);
	//Print("1\n");
	return next;
}

static void os_on_irq(int seq, int event, handler_t handler)
{
	panic_on(!IVT,"IVT wrong\n");
	panic_on(seq<0,"seq error!\n");
	panic_on(!handler,"handler error!\n");
	//kmt->spin_lock(&(IVT->ivtlock));
	panic_on(IVT->table[seq].handler,"seq repetition!\n");
	(IVT->table)[seq].event=event;
	(IVT->table)[seq].handler=handler;
	//kmt->spin_unlock(&(IVT->ivtlock));
	return;
}

MODULE_DEF(os) = {
	.init = os_init,
	.run = os_run,
	.on_irq=os_on_irq,
	.trap=os_trap,
};
