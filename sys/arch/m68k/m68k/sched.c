#include <sys/sched.h>

void _sched_save(thread_context_t* dest, thread_context_t* src) {
	*dest = *src;
}

void _sched_restore(thread_context_t* dest, thread_context_t* src) {
	*dest = *src;
}
