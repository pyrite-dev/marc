#include <sys/interrupt.h>

#include <stdint.h>

/* the 68000 vector table is the first 64 longwords of memory */
#define VECTOR_COUNT 64

extern void* m68k_vector_stubs[VECTOR_COUNT];

void _intr_init(void) {
	volatile uint32_t* vectors = (volatile uint32_t*)0;
	int		   i;

	for(i = 2; i < VECTOR_COUNT; i++) vectors[i] = (uint32_t)(uintptr_t)m68k_vector_stubs[i];

	/* lower the interrupt priority mask to 0 so that interrupts can be taken */
	__asm__ __volatile__("movew #0x2000,%%sr" ::: "cc");
}

/*
 * Install a raw handler for an exception/interrupt vector. The handler
 * replaces the generic stub, so it must save and restore whatever it uses and
 * end with rte, exactly like a CPU exception handler.
 */
void _intr_set(unsigned int vector, intr_raw_handler_t handler) {
	if(vector >= VECTOR_COUNT) return;

	((volatile uint32_t*)0)[vector] = (uint32_t)(uintptr_t)handler;
}
