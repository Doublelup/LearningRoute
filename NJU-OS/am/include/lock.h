#ifndef LOCK_H
#define LOCK_H
#include<am.h>
#include<klib-macros.h>

typedef int lock_t;

typedef enum LOCK_STATE{
    unlocked=0,
    locked=1
}lock_state;

void lock(lock_t* lock){
    atomic_xchg(lock,locked);
    return;
}

void unlock(lock_t* lock){
    atomic_xchg(lock,unlocked);
}

typedef int spinlock_t;

void spinlock_lock(spinlock_t* spinlock){
    lock_state state=locked;
    while(state!=unlocked){
        state=atomic_xchg(spinlock,locked);
    }
    return;
}

//note! invoke the function only if you held the spinlock!
void spinlock_unlock(spinlock_t* spinlock){
    atomic_xchg(spinlock,unlocked);
    return;
}

#endif
