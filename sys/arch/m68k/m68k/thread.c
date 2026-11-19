#include <sys/thread.h>

#include <string.h>

int _thread_init(thread_t* thread) {
	unsigned int base;
	unsigned int top;
	unsigned int sp;

	if(thread == NULL || thread->stack_base == NULL) {
		return -1;
	}

	base = (unsigned int)thread->stack_base;
	top  = (base + thread->stack_size) & ~3U;

	if(top < base || top - base < 22) {
		return -1;
	}

	sp = top - 8;

	((unsigned int*)sp)[0] = (unsigned int)thread_exit; /* return address */
	((unsigned int*)sp)[1] = (unsigned int)thread->arg; /* argument */

	memset(&thread->context, 0, sizeof(thread->context));

	thread->context.sp = sp;
	thread->context.sr = 0x2000; /* supervisor mode, interrupt priority mask 0 */
	thread->context.pc = (unsigned int)thread->entry;

	return 0;
}
