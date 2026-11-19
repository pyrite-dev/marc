#include <sys/spinlock.h>
#include <sys/interrupt.h>
#include <sys/sched.h>

void spinlock_init(spinlock_t* lock) {
	lock->lock = 0;
}

void spinlock_lock(spinlock_t* lock) {
	sched_preempt_disable();

	while(!_spinlock_trylock(&lock->lock)) {
		while(lock->lock != 0) {
			__BARRIER__();
		}
	}

	__BARRIER__();
}

void spinlock_unlock(spinlock_t* lock) {
	__BARRIER__();

	_spinlock_unlock(&lock->lock);

	__BARRIER__();

	sched_preempt_enable();
}

int spinlock_trylock(spinlock_t* lock) {
	sched_preempt_disable();

	if(!_spinlock_trylock(&lock->lock)) {
		sched_preempt_enable();
		return 0;
	}

	__BARRIER__();

	return 1;
}

intr_flags_t spinlock_lock_irqsave(spinlock_t* lock) {
	intr_flags_t flags = _intr_save();

	spinlock_lock(lock);

	return flags;
}

void spinlock_unlock_irqrestore(spinlock_t* lock, intr_flags_t flags) {
	spinlock_unlock(lock);

	_intr_restore(flags);
}
