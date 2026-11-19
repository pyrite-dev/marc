#ifndef __SYS_SCHED_H__
#define __SYS_SCHED_H__

#define SCHED_QUANTUM 10

#include <sys/cdefs.h>
#include <sys/waiter.h>
#include <sys/thread.h>

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
extern int sched_started;

void sched_init(void);
void sched_start(void);
void sched_add(thread_t* thread);
void sched_remove(thread_t* thread);

void sched_tick(void);
void sched_preempt(thread_context_t* context);

void sched_block(intr_flags_t flags);
void sched_wakeup(thread_t* thread);

void sched_preempt_disable(void);
void sched_preempt_enable(void);

void _sched_intr(void);
void _sched_save(thread_context_t* dest, thread_context_t* src);
void _sched_restore(thread_context_t* dest, thread_context_t* src);
#endif

__END_EXTERN_C__

#endif
