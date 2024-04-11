#include<kmt.h>

#define CHECKBLOCKEDNUM 5
struct task *spinTask=NULL;

void plinit(){
    pl=0;
}
void pllock(){
    while(atomic_xchg(&pl,1)==1);
}
void plunlock(){
    atomic_xchg(&pl,0);
}

/*fuction declaretion*/
static void kmt_spin_init(spinlock_t *lk,const char *name);
static void kmt_spin_lock(spinlock_t *lk);
static void kmt_spin_unlock(spinlock_t *lk);

static void kmt_sem_init(sem_t *sem, const char *name, int value);
static void kmt_sem_wait(sem_t *sem);
static void kmt_sem_signal(sem_t *sem);

static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg);
static void kmt_teardown(task_t *task);

static void cpu_init(void);
static void IVT_init(void);
static void stateManage_init(void);

#ifdef TESTKMT
static int tasks_init(struct tasks *tasks,const char *name);
static int recordTasks_init(recordTasks_t *recordTasks,char *name);
#else
static int tasks_init(struct tasks *tasks);
static int recordTasks_init(recordTasks_t *recordTasks);
#endif
static int tasks_enqueue(task_t *task,struct tasks *tasks);
static task_t* tasks_dequeue(struct tasks *tasks);
static int recordTasks_add(task_t *task,recordTasks_t *recordTasks);
static task_t* recordTasks_delete(task_t *task,recordTasks_t *recordTasks);


static Context *kmt_context_save(Event ev,Context *context);
static Context *kmt_schedule(Event ev,Context *context);
static Context *kmt_yield_handler(Event ev,Context *context);
static Context *kmt_syscall_handler(Event ev,Context *context);
static Context *kmt_pagefault_handler(Event ev,Context *context);
static Context *kmt_error_handler(Event ev,Context *context);

/*implement*/
void spintask(void *arg){
    while(1);
    assert(0);
    return;
}


static void cpu_init(void){
    cpu=(struct CPU*)pmm->alloc(cpu_count()*sizeof(struct CPU));
    for(int i=0;i<cpu_count();i++){
        (cpu+i)->currentTask=NULL;
        (cpu+i)->istate=true;
        (cpu+i)->ownedLockNum=0;
        (cpu+i)->cpushot=NULL;
    }
    spinTask=(struct task*)pmm->alloc(cpu_count()*sizeof(struct task));
    assert(spinTask!=NULL);
    for(int i=0;i<cpu_count();i++){
        (spinTask+i)->stack=(uint8_t*)pmm->alloc(STACKSIZE*sizeof(uint8_t));
        assert((spinTask+i)->stack!=NULL);
        (spinTask+i)->ownedSemNum=0;
        strncpy((spinTask+i)->name,"spinTask",20);
        (spinTask+i)->entry=spintask;
        Area stack=(Area){(spinTask+i)->stack,(spinTask+i)->stack+STACKSIZE};
        (spinTask+i)->context=kcontext(stack,spintask,NULL);
    }
    return;
}

static void IVT_init(void){
    IVT=(struct interruptVectorTable*)pmm->alloc(sizeof(struct interruptVectorTable));
    HELP(!IVT,"IVT_init error\n");
    //kmt_spin_init(&(IVT->ivtlock),"ivtlock");
    IVT->table=(IV*)pmm->alloc(IVTMAXSIZE*sizeof(IV));
    HELP(!IVT->table,"IVT_init error\n");
    for(int i=0;i<IVTMAXSIZE;i++){
        (IVT->table+i)->event=-1;
        (IVT->table+i)->handler=NULL;
    }
    return;
}

static void kmt_spin_init(spinlock_t *lk,const char *name){
    HELP(strlen(name)>SPINLOCKNAMESIZE,"spinlock name too long\n");
    strncpy(lk->name,name,SPINLOCKNAMESIZE);
    //Print("lockname:%s\n",lk->name);
    lk->state=0;
    lk->cpu=-1;
    return;
}

static void kmt_spin_lock(spinlock_t *lk){
    while(atomic_xchg(&lk->state,1)!=0){
        // if(ienabled()==true){
        //     yield();
        // }
    }
    if(CURRENTCPU.ownedLockNum==0)CURRENTCPU.istate=ienabled();
    iset(false);
    lk->cpu=cpu_current();
    CURRENTCPU.ownedLockNum+=1;
    return;
}

static void kmt_spin_unlock(spinlock_t *lk){
    HELP(atomic_xchg(&lk->state,1)!=1,"unlock error! lockname:%s\n",lk->name);
    HELP(lk->cpu!=cpu_current(),"own error!\n");
    atomic_xchg(&lk->state,0);
    CURRENTCPU.ownedLockNum-=1;
    HELP(CURRENTCPU.ownedLockNum<0,"unlock error!\n");
    if(CURRENTCPU.ownedLockNum==0&&CURRENTCPU.istate==true)iset(true);
    return;
}

static void kmt_init(void){
#ifdef TESTKMT
    plinit();
#endif
    cpu_init();
    IVT_init();
    stateManage_init();
    os->on_irq(IVTMIN,EVENT_NULL,kmt_context_save);
    os->on_irq(IVTMAX,EVENT_NULL,kmt_schedule);
    os->on_irq(IVTMAX-1,EVENT_ERROR,kmt_error_handler);
    os->on_irq(IVTMAX-2,EVENT_PAGEFAULT,kmt_pagefault_handler);
    os->on_irq(IVTMAX-3,EVENT_SYSCALL,kmt_syscall_handler);
    os->on_irq(IVTMAX-4,EVENT_YIELD,kmt_yield_handler);
    return;
}

/*0 create successfully, 1 error*/
static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg){
    HELP(!task,"task NULL\n");
    HELP(strlen(name)>TASKNAMESIZE,"name too long");
    //Print("kmt_create:%d\n",cpu_current());
    
    int rec=0;
    if((rec=recordTasks_add(task,aliveTasks))==0){
        task->stack=(uint8_t*)pmm->alloc(STACKSIZE*sizeof(uint8_t));
        if(task->stack==NULL){
            recordTasks_delete(task,aliveTasks);
            return 1;
        }
        kmt_spin_init(&task->stateLock,name);
        task->state=READY;
        task->ownedSemNum=0;
        strncpy(task->name,name,TASKNAMESIZE);
        task->entry=entry;
        Area stack=(Area){task->stack,task->stack+STACKSIZE};
        task->context=kcontext(stack,entry,arg);
        assert(task->context!=NULL);
        assert(tasks_enqueue(task,readyTasks)==0);
    }
    else if(rec==1)return 1;
    else assert(0);
    return 0;
}

static void kmt_teardown(task_t *task){
    if(recordTasks_delete(task,aliveTasks)==task){
        recordTasks_add(task,quitTasks);
    }
    return;
}

static void kmt_sem_init(sem_t *sem, const char *name, int value){
    assert(sem);
    assert(strlen(name)<=SEMNAMESIZE);
    strncpy(sem->name,name,SEMNAMESIZE);
    kmt_spin_init(&sem->semlock,name);
    sem->count=value;
#ifdef TESTKMT
    tasks_init(&sem->semtasks,name);
#else 
    task_init(&sem->semtasks,name);
#endif
    return;
}

static void kmt_sem_wait(sem_t *sem){
    kmt_spin_lock(&sem->semlock);
    sem->count--;
    while(sem->count<0){
        kmt_spin_lock(&CURRENTCPU.currentTask->stateLock);
        assert(CURRENTCPU.currentTask->state==RUNNING);
        CURRENTCPU.currentTask->state=BLOCKED;
        assert(tasks_enqueue(CURRENTCPU.currentTask,&sem->semtasks)==0);
        kmt_spin_unlock(&CURRENTCPU.currentTask->stateLock);
        kmt_spin_unlock(&sem->semlock);
        yield();
        kmt_spin_lock(&sem->semlock);
    }
    kmt_spin_unlock(&sem->semlock);
    return;
}

static void kmt_sem_signal(sem_t *sem){
    kmt_spin_lock(&sem->semlock);
    sem->count++;
    if(sem->count>=0){
        task_t *task=tasks_dequeue(&sem->semtasks);
        if(task){
            kmt_spin_lock(&task->stateLock);
            assert(task->state==BLOCKED);
            task->state=READY;
            kmt_spin_unlock(&task->stateLock);
        }
    }
    kmt_spin_unlock(&sem->semlock);
    return;
}

static void stateManage_init(){
    readyTasks=(struct tasks*)pmm->alloc(sizeof(struct tasks));
    blockedTasks=(struct tasks*)pmm->alloc(sizeof(struct tasks));
    aliveTasks=(recordTasks_t*)pmm->alloc(sizeof(recordTasks_t));
    quitTasks=(recordTasks_t*)pmm->alloc(sizeof(recordTasks_t));
    HELP(readyTasks==NULL,"alloc error\n");
    HELP(blockedTasks==NULL,"alloc error\n");
    HELP(aliveTasks==NULL,"alloc error\n");
    HELP(quitTasks==NULL,"alloc error\n");
#ifdef TESTKMT
    tasks_init(readyTasks,"readyTasks");
    tasks_init(blockedTasks,"blockedTasks");
    recordTasks_init(aliveTasks,"aliveTasks");
    recordTasks_init(quitTasks,"quitTasks");
#else
    tasks_init(readyTasks);
    tasks_init(blockedTasks);
    recordTasks_init(aliveTasks);
    recordTasks_init(quitTasks);
#endif   
    return;
}


/*return the state of the ope, 0 ok, 1 error*/
#ifdef TESTKMT
static int tasks_init(struct tasks *tasks,const char *name){
    HELP(!tasks,"tasks can't be NULL\n");
    HELP(strlen(name)>20,"name too long\n");

    tasksNode_t *asistNode=(tasksNode_t*)pmm->alloc(sizeof(tasksNode_t));
    HELP(!asistNode,"alloc error\n");
    strncpy(tasks->name,name,20);
    kmt_spin_init(&tasks->headLock,name);
    kmt_spin_init(&tasks->tailLock,name);

    tasks->headNode=asistNode;
    tasks->tailNode=asistNode;
    asistNode->next=NULL;
    asistNode->task=NULL;
    return 0;
}
#else
static int tasks_init(struct tasks *tasks){
    if(!tasks)return 1;

    tasksNode_t *asistNode=(tasksNode_t*)pmm->alloc(sizeof(tasksNode_t));
    HELP(!asistNode,"alloc error\n");
    if(!asistTask)return 1;

    kmt_spin_init(&tasks->headLock,name);
    kmt_spin_init(&tasks->tailLock,name);

    tasks->headNode=asistNode;
    tasks->tailNode=asistNode;
    asistNode->next=NULL;
    asistNode->task=NULL;
    return 0;
}
#endif

/*return:0 ok, 1 error*/
static int tasks_enqueue(task_t *task,struct tasks *tasks){
    assert(task!=NULL);
    assert(tasks==blockedTasks||tasks==readyTasks);
    kmt_spin_lock(&tasks->tailLock);
    assert(tasks->tailNode!=NULL);
    tasksNode_t *newNode=(tasksNode_t*)pmm->alloc(sizeof(tasksNode_t));
    if(!newNode)return 1;
    assert(newNode!=NULL);
    newNode->task=task;
    newNode->next=NULL;
    tasks->tailNode->next=newNode;
    tasks->tailNode=newNode;
    assert(tasks->tailNode!=NULL);
    kmt_spin_unlock(&tasks->tailLock);
    return 0;
}

/*return the head, if empty NULL*/
static task_t* tasks_dequeue(struct tasks *tasks){
    assert(tasks==blockedTasks||tasks==readyTasks);
    task_t *ret=NULL;
    kmt_spin_lock(&tasks->headLock);
    assert(tasks->headNode!=NULL);
    tasksNode_t *newhead=tasks->headNode->next;
    if(newhead!=NULL){//the queue is not empty
        ret=newhead->task;
        assert(ret!=NULL);
        assert(IN_RANGE((void*)(tasks->headNode),heap));
        pmm->free(tasks->headNode);
        tasks->headNode=newhead;//note the tasks->headNode is used to asisit ,don't return the headNode->task
        tasks->headNode->task=NULL;
    }
    assert(tasks->headNode!=NULL);
    kmt_spin_unlock(&tasks->headLock);
    return ret;
}

#ifdef TESTKMT
static int recordTasks_init(recordTasks_t *recordTasks,char *name){
    assert(recordTasks==aliveTasks||recordTasks==quitTasks);
    HELP(!recordTasks,"recordTasks can't be NULL\n");
    HELP(strlen(name)>20,"name too long\n");

    strncpy(recordTasks->name,name,20);
    kmt_spin_init(&recordTasks->headLock,name);
    recordTasks->headNode=NULL;
    return 0;
}
#else
static int recordTasks_init(recordTasks_t *recordTasks){
        if(!recordTasks)return 1;

        kmt_spin_init(&recordTasks->headLock);
        recordTasks->headNode=NULL;
        return 0;
}
#endif

/*return:0 ok, 1 error 2 repitition*/
static int recordTasks_add(task_t *task,recordTasks_t *recordTasks){
    assert(recordTasks==aliveTasks||recordTasks==quitTasks);
    HELP(!task,"tasks NULL\n");
    HELP(!recordTasks,"recordTasks NULL\n");
    
    //check if already exist
    kmt_spin_lock(&recordTasks->headLock);
    recordTask_t *newNode=recordTasks->headNode;
    while(newNode&&newNode->task!=task)newNode=newNode->next;
    if(newNode!=NULL){
        HELP(newNode->task!=task,"error\n");
        kmt_spin_unlock(&recordTasks->headLock);
        return 2;
    }
    
    newNode=(recordTask_t*)pmm->alloc(sizeof(recordTask_t));
    HELP(!newNode,"alloc error\n");
    newNode->task=task;
    newNode->next=recordTasks->headNode;
    recordTasks->headNode=newNode;
    kmt_spin_unlock(&recordTasks->headLock);
    return 0;
}

/*return NULL not exist in the list*/
static task_t* recordTasks_delete(task_t *task,recordTasks_t *recordTasks){
    assert(task!=NULL);
    assert(recordTasks==aliveTasks||recordTasks==quitTasks);
    task_t *ret=NULL;
    kmt_spin_lock(&recordTasks->headLock);
    recordTask_t *asistNode=recordTasks->headNode;
    recordTask_t *prior=NULL;
    while(asistNode){
        if(asistNode->task==task){
            //pick the node out
            if(prior){
                prior->next=asistNode->next;
            }
            else{
                recordTasks->headNode=recordTasks->headNode->next;
            }
            ret=asistNode->task;
            assert(IN_RANGE((void*)asistNode,heap));
            pmm->free(asistNode);
            break;
        }
        prior=asistNode;
        asistNode=asistNode->next;
    }
    kmt_spin_unlock(&recordTasks->headLock);
    return ret;
}

task_t* recordTasks_query(task_t *task,recordTasks_t *recordTasks){
    task_t *ret=NULL;
    kmt_spin_lock(&recordTasks->headLock);
    recordTask_t *asisitNode=recordTasks->headNode;
    while(asisitNode&&asisitNode->task!=task)asisitNode=asisitNode->next;
    kmt_spin_unlock(&recordTasks->headLock);
    if(asisitNode!=NULL)ret=asisitNode->task;
    return ret;
}

static Context *kmt_context_save(Event ev,Context *context){
    struct CPU *shot=(struct CPU *)pmm->alloc(sizeof(struct CPU));
    HELP(shot==NULL,"alloc error\n");
    assert(shot!=NULL);
    *shot=CURRENTCPU;
    shot->cpushot=CURRENTCPU.cpushot;
    CURRENTCPU.cpushot=shot;
    return NULL; 
}

static void cpu_restore_context(){
    struct CPU *shot=CURRENTCPU.cpushot;
    HELP(!shot,"cpushot error!\n");
    CURRENTCPU=*shot;
    assert(IN_RANGE((void*)shot,heap));
    pmm->free(shot);
    return;
}

static Context *kmt_schedule(Event ev,Context *context){
    if(CURRENTCPU.currentTask!=NULL){
        assert((uintptr_t)(context->rsp)>(uintptr_t)(CURRENTCPU.currentTask->stack));
    }
    assert(context!=NULL);
    cpu_restore_context();
    if(CURRENTCPU.currentTask!=NULL&&CURRENTCPU.currentTask!=SPINTASK){
        //save the context into task, move the currentTask into tasks
        kmt_spin_lock(&CURRENTCPU.currentTask->stateLock);
        enum taskstate stateshot=CURRENTCPU.currentTask->state;
        kmt_spin_unlock(&CURRENTCPU.currentTask->stateLock);
        if(stateshot==RUNNING){
            if(CURRENTCPU.currentTask->ownedSemNum==0&&recordTasks_delete(CURRENTCPU.currentTask,quitTasks)==CURRENTCPU.currentTask){
                //in quitTasks, just free
                assert(IN_RANGE((void*)(CURRENTCPU.currentTask->stack),heap));
                pmm->free(CURRENTCPU.currentTask->stack);
                CURRENTCPU.currentTask->stack=NULL;
                assert(IN_RANGE((void*)(CURRENTCPU.currentTask),heap));
                pmm->free(CURRENTCPU.currentTask);
                CURRENTCPU.currentTask=NULL;
            }
            else{
                CURRENTCPU.currentTask->context=context;
                kmt_spin_lock(&CURRENTCPU.currentTask->stateLock);
                CURRENTCPU.currentTask->state=READY;
                kmt_spin_unlock(&CURRENTCPU.currentTask->stateLock);
                HELP(tasks_enqueue(CURRENTCPU.currentTask,readyTasks)!=0,"enqueue error!\n");
                CURRENTCPU.currentTask=NULL;
            }
        }
        else if(stateshot==BLOCKED){
            CURRENTCPU.currentTask->context=context;
            HELP(tasks_enqueue(CURRENTCPU.currentTask,blockedTasks)!=0,"enqueue error!\n");
            CURRENTCPU.currentTask=NULL;
        }
        else if(stateshot==READY){
            CURRENTCPU.currentTask->context=context;
            HELP(tasks_enqueue(CURRENTCPU.currentTask,readyTasks)!=0,"enqueue error!\n");
            CURRENTCPU.currentTask=NULL;
        }
        else HELP(1,"state error!\n");
    }
    else{
        if(CURRENTCPU.currentTask==SPINTASK){
            SPINTASK->context=context;
            CURRENTCPU.currentTask=NULL;
        }
    }
    //move the waked task into readyTasks
    task_t *asistTask=NULL;
    for(int i=0;i<CHECKBLOCKEDNUM;i++){
        asistTask=tasks_dequeue(blockedTasks);
        if(asistTask==NULL)break;

        assert(0);
        
        kmt_spin_lock(&asistTask->stateLock);
        if(asistTask->state==READY){
            kmt_spin_unlock(&asistTask->stateLock);
            HELP(tasks_enqueue(asistTask,readyTasks)!=0,"enqueue error!\n");
            break;
        }
        kmt_spin_unlock(&asistTask->stateLock);
        HELP(tasks_enqueue(asistTask,blockedTasks)!=0,"enqueue error!\n");
        asistTask=NULL;
    }
    if(asistTask!=NULL){
        HELP(tasks_enqueue(asistTask,readyTasks)!=0,"enqueue error!\n");
    }
    //pick up a ready task
    CURRENTCPU.currentTask=tasks_dequeue(readyTasks);
    if(CURRENTCPU.currentTask){
        kmt_spin_lock(&CURRENTCPU.currentTask->stateLock);
        CURRENTCPU.currentTask->state=RUNNING;
        kmt_spin_unlock(&CURRENTCPU.currentTask->stateLock);
        assert(CURRENTCPU.currentTask->context!=NULL);
        return CURRENTCPU.currentTask->context;
    }
    else {
        CURRENTCPU.currentTask=SPINTASK;
        return SPINTASK->context;
    }
    
}

static Context *kmt_yield_handler(Event ev,Context *context){
    // cpu_restore_context();
    // return context;
    return NULL;
}

static Context *kmt_syscall_handler(Event ev,Context *context){
    cpu_restore_context();
    return context;
}

static Context *kmt_pagefault_handler(Event ev,Context *context){
    cpu_restore_context();
    return context;
}

static Context *kmt_error_handler(Event ev,Context *context){
    cpu_restore_context();
    return context;
}


MODULE_DEF(kmt) = {
	.init = kmt_init,
	.create = kmt_create,
    .teardown = kmt_teardown,
    .spin_init = kmt_spin_init,
    .spin_lock = kmt_spin_lock,
    .spin_unlock = kmt_spin_unlock,
    .sem_init = kmt_sem_init,
    .sem_wait = kmt_sem_wait,
    .sem_signal = kmt_sem_signal,
};



