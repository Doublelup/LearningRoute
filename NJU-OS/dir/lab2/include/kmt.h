#ifndef KMP_H
#define KMP_H
#include<common.h>

#define SPINLOCKNAMESIZE (20)

struct spinlock{
    char    name[SPINLOCKNAMESIZE];
    int     state;//0 unlocked, 1 locked
};

struct tasks{
    struct spinlock taskslock;
    task_t *headTask;
    task_t *tailTask;
};

struct tasks *runningTasks;
struct tasks *readyTasks;//queue
struct tasks *blockedTasks;

struct recordTask{
    task_t *task;
    struct recordTask *next;
};

struct recordTasks{
    struct spinlock taskslock;
    struct recordTask *headTask;
};

struct recordTasks *aliveTasks;
struct recordTasks *quitTasks;

#define TASKNAMESIZE (20)
#define STACKSIZE (1024)

enum taskstate{
    RUNNING=0,
    READY,
    BLOCKED,
};

struct task{
    int                 index;
    struct spinlock     tasklock;//only protect the state of the task
    enum taskstate      state;
    int                 ownedSemNum;
    char                name[TASKNAMESIZE];
    struct task        *next;
    void              (*entry)(void*);//kernel thread doesn't return?
    Context             context;
    uint8_t            *stack;
};



#define SEMNAMESIZE (20)

struct semaphore{
    char                 name[SEMNAMESIZE+1];
    struct spinlock      semlock;
    int                  value;
    int                  count;
    struct recordTask   *waitqueuehead;
    struct recordTask   *waitqueuetail;
};

struct spinlock emptyCtxLock;
Context emptyCtx;

#endif