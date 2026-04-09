#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"
 
#define MAX_THREADS  16
#define STACK_SIZE   (4096 * 4) 
 
struct ucontext {
    uint esp;
};

typedef enum {
	TS_FREE     = 0,
	TS_RUNNING  = 1,
	TS_RUNNABLE = 2,
	TS_DONE     = 3,
	TS_JOINED   = 4,
} tstate_t;

typedef struct {
	struct ucontext *ctx;
 	tstate_t         state;
	char            *stack;
	void           (*fn)(void *);
	void            *arg;
} tcb_t;

static tcb_t threads[MAX_THREADS];
static int   current    = 0;
static int   initialised = 0;
 
void swtch(struct ucontext **old, struct ucontext *new);
 
//thread start
static void
thread_trampoline(void)
{
    tcb_t *t = &threads[current];
    t->fn(t->arg);
    t->state = TS_DONE;
    while (1) //until join
        thread_yield();
}
 
static void
schedule(void)
{
    int old_idx = current;
 
    for (int i = 1; i <= MAX_THREADS; i++) {
        int idx = (old_idx + i) % MAX_THREADS;
        if (threads[idx].state == TS_RUNNABLE) {
            if (threads[old_idx].state == TS_RUNNING)
                threads[old_idx].state = TS_RUNNABLE;
            threads[idx].state = TS_RUNNING;
            current = idx;
            swtch(&threads[old_idx].ctx, threads[idx].ctx);
            return;
        }
    }
}
 
//API
void
thread_init(void)
{
    if (initialised) return;
    initialised = 1;
 
    for (int i = 0; i < MAX_THREADS; i++) {
        threads[i].state = TS_FREE;
        threads[i].stack = 0;
        threads[i].ctx   = 0;
    }

    current = 0;
    threads[0].state = TS_RUNNING;
}
 
tid_t
thread_create(void (*fn)(void *), void *arg)
{
    int slot = -1;
    for (int i = 1; i < MAX_THREADS; i++) {
        if (threads[i].state == TS_FREE ||
            threads[i].state == TS_JOINED) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;
 
    //allocation if none
    if (threads[slot].stack == 0) {
        threads[slot].stack = (char *)malloc(STACK_SIZE);
        if (threads[slot].stack == 0) return -1;
    }
 
    threads[slot].fn  = fn;
    threads[slot].arg = arg;
 
	  //the stack grows down towards the trampoline
    uint *sp = (uint *)(threads[slot].stack + STACK_SIZE);
	*--sp = (uint)thread_trampoline; //ret
    *--sp = 0; //ebp
    *--sp = 0; //ebx
    *--sp = 0; //esi
    *--sp = 0; //edi
    threads[slot].ctx = (struct ucontext *)sp;
 
    threads[slot].state = TS_RUNNABLE;
    return (tid_t)slot;
}
 
void
thread_yield(void)
{
    schedule();
}
 
int
thread_join(tid_t tid)
{
    if (tid <= 0 || tid >= MAX_THREADS)  return -1;
    if (threads[tid].state == TS_FREE)   return -1;
 
    while (threads[tid].state != TS_DONE &&
           threads[tid].state != TS_JOINED) {
        thread_yield();
    }
 
    if (threads[tid].state == TS_JOINED) return -1;
 
    threads[tid].state = TS_JOINED;
    return 0;
}
