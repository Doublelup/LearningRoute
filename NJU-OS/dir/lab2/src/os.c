#include <os.h>

static void os_init()
{
	pmm->init();
	kmt->init();
}

static void os_run()
{
	iset(true);
	while(1);
	panic_on(1,"os_run return!\n");
}

static Context* os_trap(Event ev,Context *context)
{
	Context *next=NULL;
	cpu[cpu_current()].ownedLockNum++;
	for(int i=0;i<IVTMAXSIZE;i++){
		if((IVT->table)[i].event==EVENT_NULL||(IVT->table)[i].event==ev.event){
			Context *ctx=(IVT->table)[i].handler(ev,context);
			if(ctx)next=ctx;
		}
	}
	cpu[cpu_current()].ownedLockNum--;
	panic_on(cpu[cpu_current()].ownedLockNum!=0,"lock error!\n");
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
