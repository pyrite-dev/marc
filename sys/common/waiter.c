#include <sys/waiter.h>
#include <sys/spinlock.h>
#include <sys/thread.h>
#include <sys/interrupt.h>
#include <sys/sched.h>

#include <stddef.h>

static void waiter_insert(waiter_t* waiter, thread_t* thread) {
	thread->waiting	  = waiter;
	thread->wait_prev = waiter->tail;
	thread->wait_next = NULL;

	if(waiter->tail == NULL) {
		waiter->head = thread;
	} else {
		waiter->tail->wait_next = thread;
	}

	waiter->tail = thread;
}

static void waiter_remove(waiter_t* waiter, thread_t* thread) {
	if(thread->wait_prev == NULL) {
		waiter->head = thread->wait_next;
	} else {
		thread->wait_prev->wait_next = thread->wait_next;
	}

	if(thread->wait_next == NULL) {
		waiter->tail = thread->wait_prev;
	} else {
		thread->wait_next->wait_prev = thread->wait_prev;
	}

	thread->waiting	  = NULL;
	thread->wait_prev = NULL;
	thread->wait_next = NULL;
}

void waiter_init(waiter_t* waiter) {
	spinlock_init(&waiter->lock);

	waiter->head = NULL;
	waiter->tail = NULL;
}

void waiter_sleep(waiter_t* waiter, spinlock_t* lock, intr_flags_t flags) {
	thread_t* thread = thread_current();

	spinlock_lock(&waiter->lock);
	waiter_insert(waiter, thread);
	thread->state = THREAD_WAITING;
	spinlock_unlock(&waiter->lock);

	spinlock_unlock(lock);

	sched_block(flags);

	spinlock_lock_irqsave(lock);
}

void waiter_wakeup(waiter_t* waiter) {
	thread_t*    thread;
	intr_flags_t flags = spinlock_lock_irqsave(&waiter->lock);

	while((thread = waiter->head) != NULL) {
		waiter_remove(waiter, thread);

		if(thread->state == THREAD_WAITING) sched_wakeup(thread);
	}

	spinlock_unlock_irqrestore(&waiter->lock, flags);
}

void waiter_wakeup_one(waiter_t* waiter) {
	thread_t*    thread;
	intr_flags_t flags = spinlock_lock_irqsave(&waiter->lock);

	thread = waiter->head;

	if(thread != NULL) {
		waiter_remove(waiter, thread);

		if(thread->state == THREAD_WAITING) sched_wakeup(thread);
	}

	spinlock_unlock_irqrestore(&waiter->lock, flags);
}
