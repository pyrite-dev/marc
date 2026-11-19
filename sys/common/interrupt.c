#include <sys/interrupt.h>
#include <machine/interrupt.h>

#include <string.h>

#define NVEC 256

struct intr_entry {
	intr_handler_t handler;
	void*	       arg;
	int	       enabled;
};

static struct intr_entry intr_table[NVEC];

void intr_init(void) {
	memset(intr_table, 0, sizeof(intr_table));
	_intr_init();
}

int intr_register(unsigned int vector, intr_handler_t handler, void* arg) {
	if(vector >= NVEC) return -1;
	if(handler == NULL) {
		intr_unregister(vector);
		return 0;
	}

	intr_table[vector].handler = handler;
	intr_table[vector].arg	   = arg;
	intr_table[vector].enabled = 0;

	return 0;
}

void intr_unregister(unsigned int vector) {
	if(vector >= NVEC) return;

	intr_table[vector].enabled = 0;
	intr_table[vector].handler = NULL;
	intr_table[vector].arg	   = NULL;
}

void intr_enable(unsigned int vector) {
	if(vector >= NVEC) return;
	if(intr_table[vector].handler == NULL) return;

	intr_table[vector].enabled = 1;
}

void intr_disable(unsigned int vector) {
	if(vector >= NVEC) return;
	if(intr_table[vector].handler == NULL) return;

	intr_table[vector].enabled = 0;
}

#ifdef __i960__
__attribute__((interrupt))
#endif
void intr_dispatch(unsigned int vector, struct iregs* iregs) {
	struct intr_entry* entry;

	if(vector >= NVEC) return;

	entry = &intr_table[vector];

	if(entry->handler == NULL || !entry->enabled) return;

	entry->handler(vector, iregs, entry->arg);
}
