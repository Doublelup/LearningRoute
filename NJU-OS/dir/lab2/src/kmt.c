#include<kmt.h>

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
static void tasks_init(void);

static void task_append(task_t* task,struct tasks* tasks);
static task_t* task_delete(task_t* task,struct tasks *tasks);
//static task_t* task_search(task_t* task,struct tasks *tasks);

static struct recordTask* recordTask_search(task_t *obj,struct recordTask *recordtasks);
static Context *kmt_context_save(Event ev,Context *context);
static Context *kmt_schedule(Event ev,Context *context);

static struct recordTask* recordTask_search(task_t *obj,struct recordTask *recordtasks)
{
    panic_on(!obj,"error!\n");
    struct recordTask *asist=recordtasks;
    while(asist!=NULL&&obj!=asist->task){
        asist=asist->next;
    }
    return asist;
}

static void cpu_init(void){
    cpu=(struct CPU*)pmm->alloc(cpu_count()*sizeof(struct CPU));
    panic_on(!cpu,"cpu_init error!\n");
    for(int i=0;i<cpu_count();i++){
        (cpu+i)->currentTask=NULL;
        (cpu+i)->ownedLockNum=0;
    }
    return;
}

static void IVT_init(void){
    IVT=(struct interruptVectorTable*)pmm->alloc(sizeof(struct interruptVectorTable));
    panic_on(!IVT,"IVT_init error\n");
    //kmt_spin_init(&(IVT->ivtlock),"ivtlock");
    IVT->table=(IV*)pmm->alloc(IVTMAXSIZE*sizeof(IV));
    panic_on(!IVT->table,"IVT_init error\n");
    for(int i=0;i<IVTMAXSIZE;i++){
        (IVT->table+i)->event=-1;
        (IVT->table+i)->handler=NULL;
    }
    return;
}

static void kmt_spin_init(spinlock_t *lk,const char *name){
    panic_on(!lk,"lk error!\n");
    strncpy(lk->name,name,SPINLOCKNAMESIZE);
    lk->state=0;
    //lk->owner=-1;//-1 means no cpu own the lock
    return;
}

static void kmt_spin_lock(spinlock_t *lk){
    iset(false);
    while(atomic_xchg(&(lk->state),1)==1);
    cpu[cpu_current()].ownedLockNum+=1;
    return;
}

static void kmt_spin_unlock(spinlock_t *lk){
    panic_on(lk->state==0,"lock error!\n");
    atomic_xchg(&(lk->state),0);
    cpu[cpu_current()].ownedLockNum-=1;
    panic_on(cpu[cpu_current()].ownedLockNum<0,"spin_unlock error\n");
    panic_on(ienabled()==true,"iset state error!\n");
    if(cpu[cpu_current()].ownedLockNum==0)iset(true);
}

static void kmt_init(void){
    kmt_spin_init(&emptyCtxLock,"emptyCtxLock");
    cpu_init();
    IVT_init();
    tasks_init();
    os->on_irq(IVTMIN,EVENT_NULL,kmt_context_save);
    os->on_irq(IVTMAX,EVENT_NULL,kmt_schedule);
    return;
}


static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg){
    int ret=0;
#ifndef TESTKMT
    if(!task||strlen(name)>TASKNAMESIZE)return ret;
#else
    panic_on(!task||strlen(name)>TASKNAMESIZE,"kmt_create error!\n");
#endif
    task->stack=(uint8_t*)pmm->alloc(STACKSIZE*sizeof(uint8_t));
    if(!task->stack){//alloc error
        return ret;
    }
    task->index=(uintptr_t)task;//choose the location to avoid repetition
    strncpy(task->name,name,TASKNAMESIZE);
    kmt_spin_init(&(task->tasklock),name);
    task->state=READY;
    task->ownedSemNum=0;
    task->entry=entry;
    Area stack=(Area){task->stack,task->stack+STACKSIZE};
    task->context=*kcontext(stack,entry,arg);
    task->next=NULL;
    //add to readyTasks
    kmt_spin_lock(&readyTasks->taskslock);
    task_append(task,readyTasks);
    kmt_spin_unlock(&readyTasks->taskslock);
    //add to aliveTasks
    kmt_spin_lock(&aliveTasks->taskslock);
    struct recordTask *newTaskNode=(struct recordTask*)pmm->alloc(sizeof(struct recordTask));
    panic_on(!newTaskNode,"newTask create error!\n");
    newTaskNode->task=task;
    newTaskNode->next=aliveTasks->headTask;
    aliveTasks->headTask=newTaskNode;
    kmt_spin_unlock(&aliveTasks->taskslock);
    
    return ret;
}

static void kmt_teardown(task_t *task){
    kmt_spin_lock(&aliveTasks->taskslock);
    struct recordTask* asist=aliveTasks->headTask;
    struct recordTask* prior=NULL;
    while(asist!=NULL&&asist->task!=task){
        prior=asist;
        asist=asist->next;
    }
    if(asist!=NULL){
        if(prior){
            prior->next=asist->next;
            asist->next=NULL;
        }
        else{
            aliveTasks->headTask=asist->next;
            asist->next=NULL;
        }
    }
    kmt_spin_lock(&aliveTasks->taskslock);
    if(asist!=NULL){
        kmt_spin_lock(&quitTasks->taskslock);
        asist->next=quitTasks->headTask;
        quitTasks->headTask=asist;
        kmt_spin_unlock(&quitTasks->taskslock);
    }
    return;
}

static void kmt_sem_init(sem_t *sem, const char *name, int value){
#ifndef TESTKMT
    if(strlen(name)>SEMNAMESIZE)return;
    if(value<=0)return;
#else
    panic_on(strlen(name)>SEMNAMESIZE,"sem name too long!\n");
    panic_on(value<=0,"value error!\n");
#endif
    strncpy(sem->name,name,SEMNAMESIZE);
    kmt_spin_init(&(sem->semlock),name);
    sem->value=value;
    sem->count=value;
    sem->waitqueuehead=NULL;
    sem->waitqueuetail=NULL;
    return;
}

static void kmt_sem_wait(sem_t *sem){
    int flag=0;
    kmt_spin_lock(&sem->semlock);
    if(sem->value>0){//aquire the sem
        sem->value--;
        kmt_spin_lock(&cpu[cpu_current()].currentTask->tasklock);
        cpu[cpu_current()].currentTask->ownedSemNum++;
        kmt_spin_unlock(&cpu[cpu_current()].currentTask->tasklock);
        flag=1;
    }
    else{//join the waitqueue
        //insert the task into queue
        if(sem->waitqueuehead==NULL){//queue empty
            panic_on(sem->waitqueuetail,"queue error!\n");
            struct recordTask *nodeptr=(struct recordTask*)pmm->alloc(sizeof(struct recordTask));
            nodeptr->task=cpu[cpu_current()].currentTask;
            nodeptr->next=NULL;
            sem->waitqueuehead=nodeptr;
            sem->waitqueuetail=nodeptr;
        }
        else{
            struct recordTask *nodeptr=(struct recordTask *)pmm->alloc(sizeof(struct recordTask));
            nodeptr->task=cpu[cpu_current()].currentTask;
            nodeptr->next=NULL;
            sem->waitqueuetail->next=nodeptr;
            sem->waitqueuetail=nodeptr;
        }
    }
    kmt_spin_unlock(&sem->semlock);
    if(flag==0){//fail to aquire the sem, change the state of the task
        kmt_spin_lock(&cpu[cpu_current()].currentTask->tasklock);
        //panic_on(cpu[cpu_current()].currentTask->state!=RUNNING,"task state error!\n");
        if(cpu[cpu_current()].currentTask->state==RUNNING)cpu[cpu_current()].currentTask->state=BLOCKED;//avoid the situation that some thread invoke signal after unlock (the task is already in the wait queue), and be run on another cpu!
        else{panic_on(cpu[cpu_current()].currentTask->state!=READY,"error!\n");}
        kmt_spin_unlock(&cpu[cpu_current()].currentTask->tasklock);
        yield();
    }
    return;
}

static void kmt_sem_signal(sem_t *sem){
    panic_on(!sem,"sem_signal error!\n");
    kmt_spin_lock(&sem->semlock);

    sem->count++;
    struct recordTask *objnode=sem->waitqueuehead;
    //assert that the sem->waitqueuehead must alive
    if(objnode!=NULL){    
        kmt_spin_lock(&aliveTasks->taskslock);
        panic_on(recordTask_search(objnode->task,aliveTasks->headTask)==NULL,"error!\n");
        kmt_spin_unlock(&aliveTasks->taskslock);

        kmt_spin_lock(&objnode->task->tasklock);
        objnode->task->state=READY;
        kmt_spin_unlock(&objnode->task->tasklock);

        if(sem->waitqueuehead==sem->waitqueuetail){
            sem->waitqueuehead=NULL;
            sem->waitqueuetail=NULL;
        }
        else{
            sem->waitqueuehead=objnode->next;
        }
        pmm->free(objnode);
    }

    kmt_spin_unlock(&sem->semlock);
    return;
}

static void tasks_init(){
    //init readyTasks
    readyTasks=(struct tasks*)pmm->alloc(sizeof(struct tasks));
    panic_on(!readyTasks,"tasks_init error!\n");
    
    //init tasks
    kmt_spin_init(&readyTasks->taskslock,"readyTasks");
    readyTasks->headTask=NULL;
    readyTasks->tailTask=NULL;

    blockedTasks=(struct tasks*)pmm->alloc(sizeof(struct tasks));
    panic_on(!blockedTasks,"tasks_init error!\n");
    kmt_spin_init(&blockedTasks->taskslock,"blockedTasks");
    blockedTasks->headTask=NULL;
    blockedTasks->tailTask=NULL;

    runningTasks=(struct tasks*)pmm->alloc(sizeof(struct tasks));
    panic_on(!runningTasks,"tasks_init error!\n");
    kmt_spin_init(&runningTasks->taskslock,"runningTasks");
    runningTasks->headTask=NULL;
    runningTasks->tailTask=NULL;

    quitTasks=(struct recordTasks*)pmm->alloc(sizeof(struct recordTasks));
    panic_on(!quitTasks,"tasks_init error\n");
    kmt_spin_init(&quitTasks->taskslock,"quitTasks");
    quitTasks->headTask=NULL;

    aliveTasks=(struct recordTasks*)pmm->alloc(sizeof(struct recordTasks));
    panic_on(!aliveTasks,"tasks_init error!\n");
    kmt_spin_init(&aliveTasks->taskslock,"aliveTasks");
    aliveTasks->headTask=NULL;

    return;
}

static void task_append(task_t *task,struct tasks* tasks){//insert after the headTask
    panic_on(!task||!tasks,"task_append error!\n");
    //kmt_spin_lock(&tasks->taskslock);
    if(tasks->headTask==NULL){//empty queue
        panic_on(tasks->tailTask!=NULL,"queue error!\n");
        task->next=NULL;
        tasks->headTask=task;
        tasks->tailTask=task;
    }
    else{
        panic_on(tasks->tailTask==NULL,"queue error\n");
        tasks->tailTask->next=task;
        task->next=NULL;
        tasks->tailTask=task;
    }
    //kmt_spin_unlock(&tasks->taskslock);
    return;
}

static task_t* task_delete(task_t *task,struct tasks *tasks){
    panic_on(!task||!tasks,"task_delete error!\n");
    //kmt_spin_lock(&tasks->taskslock);

    task_t *ret=NULL;
    task_t *prior=NULL;
    task_t *objtask=tasks->headTask;
    while(objtask&&objtask!=task){
        prior=objtask;
        objtask=objtask->next;
    }
    if(objtask==task){//exist in the queue
        ret=task;
        if(prior){//prior exist, not the headTask
            if(task!=tasks->tailTask){
                prior->next=objtask->next;
                objtask->next=NULL;
            }
            else{
                prior->next=objtask->next;
                objtask->next=NULL;
                tasks->tailTask=prior;
            }
        }
        else{
            if(task!=tasks->tailTask){
                tasks->headTask=objtask->next;
                objtask->next=NULL;
            }
            else{
                tasks->headTask=NULL;
                tasks->tailTask=NULL;
            }
        }
    }

    //kmt_spin_unlock(&tasks->taskslock);
    return ret;
}

//notice: you may hope search and use be atomic, so you have to lock and unlock the taskslock! 
// task_t* task_search(task_t *task,struct tasks *tasks){//if the task is still in tasks, return task(locked), else return NULL
//     panic_on(!task||!tasks,"task_search error!\n");

//     //kmt_spin_lock(&tasks->taskslock);
//     task_t *ret=NULL;
//     task_t *objtask=tasks->headTask;

//     while(objtask&&objtask!=task){
//         objtask=objtask->next;
//     }

//     if(objtask==task){
//         ret=objtask;
//     }

//     //kmt_spin_unlock(&tasks->taskslock);
//     return ret;
// }

static Context *kmt_context_save(Event ev,Context *context){
    if(cpu[cpu_current()].currentTask!=NULL){
        task_t *currentTask=cpu[cpu_current()].currentTask;
        kmt_spin_lock(&currentTask->tasklock);
        if(currentTask->state==RUNNING){
            if(currentTask->ownedSemNum==0){
                kmt_spin_unlock(&currentTask->tasklock);
            
                //search quitTasks
                kmt_spin_lock(&quitTasks->taskslock);
                struct recordTask *asist=quitTasks->headTask;
                struct recordTask *prior=NULL;
                while(asist&&asist->task!=currentTask){
                    prior=asist;
                    asist=asist->next;
                }
                if(asist!=NULL){//if exist in the quitTasks, then delete the quitTask node
                    if(prior){
                        prior->next=asist->next;
                        asist->next=NULL;
                    }
                    else{
                        quitTasks->headTask=asist->next;
                    }
                    pmm->free(asist);
                }
                kmt_spin_unlock(&quitTasks->taskslock);
                
                if(asist!=NULL){
                    //remove from runningTasks
                    kmt_spin_lock(&runningTasks->taskslock);
                    panic_on(task_delete(currentTask,runningTasks)!=currentTask,"error!\n");
                    kmt_spin_unlock(&runningTasks->taskslock);

                    //free the task
                    pmm->free(currentTask->stack);
                    pmm->free(currentTask);
                }
            }
            else{
                currentTask->state=READY;
                kmt_spin_unlock(&currentTask->tasklock);

                kmt_spin_lock(&runningTasks->taskslock);
                task_append(currentTask,runningTasks);
                kmt_spin_unlock(&runningTasks->taskslock);
            }
        }
        else if(currentTask->state==BLOCKED){
            kmt_spin_unlock(&currentTask->tasklock);
            
            kmt_spin_lock(&runningTasks->taskslock);
            panic_on(task_delete(currentTask,runningTasks)!=currentTask,"error!\n");
            kmt_spin_unlock(&runningTasks->taskslock);

            kmt_spin_lock(&blockedTasks->taskslock);
            task_append(currentTask,blockedTasks);
            kmt_spin_unlock(&blockedTasks->taskslock);
        }
        else{
            panic_on(currentTask->state!=READY,"error\n");
            kmt_spin_unlock(&currentTask->tasklock);

            kmt_spin_lock(&runningTasks->taskslock);
            panic_on(task_delete(currentTask,runningTasks)!=currentTask,"error!\n");
            kmt_spin_unlock(&runningTasks->taskslock);

            kmt_spin_lock(&readyTasks->taskslock);
            task_append(currentTask,readyTasks);
            kmt_spin_unlock(&readyTasks->taskslock);            
        }
    }
    else{
        kmt_spin_lock(&emptyCtxLock);
        emptyCtx=*context;
        kmt_spin_unlock(&emptyCtxLock);
    }

    cpu[cpu_current()].currentTask=NULL;
    return NULL;
}

static Context *kmt_schedule(Event ev,Context *context){
    task_t *task=NULL;
    Context *ret=NULL;
    kmt_spin_lock(&readyTasks->taskslock);
    if(readyTasks->headTask!=NULL){
        if(readyTasks->headTask==readyTasks->tailTask){
            task=readyTasks->headTask;
            readyTasks->headTask=NULL;
            readyTasks->tailTask=NULL;
        }
        else{
            task=readyTasks->headTask;
            readyTasks->headTask=readyTasks->headTask->next;
        }
    }
    kmt_spin_unlock(&readyTasks->taskslock);
    if(task!=NULL){
        kmt_spin_lock(&runningTasks->taskslock);
        task_append(task,runningTasks);
        kmt_spin_unlock(&runningTasks->taskslock);
        
        cpu[cpu_current()].currentTask=task;
        ret=&task->context;
    }
    else{
        ret=&emptyCtx;
    }
    return ret;
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