#ifndef KMP_H
#define KMP_H
#include<common.h>

#define SPINLOCKNAMESIZE (20)

struct spinlock{
    char    name[SPINLOCKNAMESIZE];
    int     cpu;
    int     state;//0 unlocked, 1 locked
#ifdef ENHENCED
    int     guard;//0 unlocked,1 locked
    struct recordTasks *headTask;
#endif
};

typedef struct tasksNode{
    task_t              *task;
    struct tasksNode    *next;
}tasksNode_t;

struct tasks{//queue
#ifdef TESTKMT
    char            name[20];
#endif
    //struct spinlock taskslock;
    tasksNode_t     *headNode;
    struct spinlock headLock;
    tasksNode_t     *tailNode;
    struct spinlock tailLock;
};

//struct tasks *runningTasks;
struct tasks *readyTasks;
struct tasks *blockedTasks;

typedef struct recordTask{
    task_t              *task;
    struct recordTask   *next;
}recordTask_t;

typedef struct recordTasks{
#ifdef TESTKMT
    char                name[20];
#endif
    struct spinlock headLock;
    struct recordTask *headNode;
}recordTasks_t;

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
    struct spinlock     stateLock;//only protect the state of the task
    enum taskstate      state;
    int                 ownedSemNum;
    char                name[TASKNAMESIZE];
    void              (*entry)(void*);//kernel thread doesn't return?
    Context            *context;
    uint8_t            *stack;
};

extern struct task *spinTask;
#define SPINTASK (spinTask+cpu_current())



#define SEMNAMESIZE (20)

struct semaphore{
    char                 name[SEMNAMESIZE+1];
    struct spinlock      semlock;
    int                  count;
    struct tasks         semtasks;
};

task_t* recordTasks_query(task_t *task,recordTasks_t *recordTasks);

#endif