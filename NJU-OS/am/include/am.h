#ifndef AM_H__
#define AM_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include ARCH_H // this macro is defined in $CFLAGS
                // examples: "arch/x86-qemu.h", "arch/native.h", ...

// Memory protection flags
#define MMAP_NONE  0x00000000 // no access
#define MMAP_READ  0x00000001 // can read
#define MMAP_WRITE 0x00000002 // can write

// Memory area for [@start, @end)
typedef struct {
  void *start, *end;
} Area;

// Arch-dependent processor context
typedef struct Context Context;//defined in x86_64-qemu.h

// An event of type @event, caused by @cause of pointer @ref
typedef struct {
  enum {
    EVENT_NULL = 0,
    EVENT_YIELD, EVENT_SYSCALL, EVENT_PAGEFAULT, EVENT_ERROR,
    EVENT_IRQ_TIMER, EVENT_IRQ_IODEV,
  } event;//tell you which kind of interrupt
  uintptr_t cause, ref;
  const char *msg;
} Event;

// A protected address space with user memory @area
// and arch-dependent @ptr
typedef struct {
  int pgsize;
  Area area;
  void *ptr;
} AddrSpace;

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------- TRM: Turing Machine -----------------------
extern   Area        heap;
void     putch       (char ch);
void     halt        (int code) __attribute__((__noreturn__));

// -------------------- IOE: Input/Output Devices --------------------
bool     ioe_init    (void);
void     ioe_read    (int reg, void *buf);
void     ioe_write   (int reg, void *buf);
#include "amdev.h"

// ---------- CTE: Interrupt Handling and Context Switching ----------
bool     cte_init    (Context *(*handler)(Event ev, Context *ctx));//init how to handle interrupt, $ev tell you which kind of interrupt it is, and $ctx is the context(registers or so not handled yet), return a context* point at the context that need to be process after the interrupt;
void     yield       (void);//hand over CPU (kind like invoke interrupt)
bool     ienabled    (void);//is interrupt enabled, true for yes, false for no
void     iset        (bool enable);//turn on or turn off the IF
Context *kcontext    (Area kstack, void (*entry)(void *), void *arg);//create the primiry state for a state machine; $kstack refers to the stack of the process, $entry point at the begin of the process, $arg is the arg for the entry

// ----------------------- VME: Virtual Memory -----------------------
bool     vme_init    (void *(*pgalloc)(int), void (*pgfree)(void *));
void     protect     (AddrSpace *as);
void     unprotect   (AddrSpace *as);
void     map         (AddrSpace *as, void *vaddr, void *paddr, int prot);
Context *ucontext    (AddrSpace *as, Area kstack, void *entry);

// ---------------------- MPE: Multi-Processing ----------------------
bool     mpe_init    (void (*entry)());//init numbers of processers(the exactly num is passed by smp when type "make run smp=?" at the dash board), and every processer invoke the function: entry
int      cpu_count   (void);//? to get the number of the CPU
int      cpu_current (void);//?to get the id of the CPU that execute the function
int      atomic_xchg (int *addr, int newval);

#ifdef __cplusplus
}
#endif

#endif
