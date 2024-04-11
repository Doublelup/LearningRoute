#include<test.h>

void subtask(void* arg){
    printf("hello\n");
    while(1);
    return;
}

void task1(void* args){
    char* words=(char*)args;
    printf(words);
    task_t* t1=(task_t*)pmm->alloc(sizeof(task_t));
    task_t* t2=(task_t*)pmm->alloc(sizeof(task_t));
    //Print("t1:%p\tt2:%p\n",t1,t2);
    kmt->create(t1,"t1",subtask,NULL);
    kmt->create(t2,"t2",subtask,NULL);
    // Print("taskid:%d\n",taskid1);
    // Print("taskid:%d\n",taskid2);
    for(volatile int i=0;i<1000000;i++);
    kmt->teardown(t1);
    kmt->teardown(t2);
    assert(recordTasks_query(t1,aliveTasks)==NULL);
    assert(recordTasks_query(t2,aliveTasks)==NULL);
    //Print(words);
    while(1);
    return;
}

//void task2(void* args);