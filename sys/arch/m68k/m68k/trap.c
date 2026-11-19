#include <sys/interrupt.h>
#include <sys/cons.h>
#include <sys/panic.h>
#include <sys/sched.h>

#include <stdint.h>

#define VECTOR_BUS_ERROR 2
#define VECTOR_ADDRESS_ERROR 3
#define VECTOR_SPURIOUS 24
#define VECTOR_AUTOVECTOR_1 25
#define VECTOR_AUTOVECTOR_7 31
#define VECTOR_TRAP_0 32
#define VECTOR_TRAP_15 47

/* number of autovector interrupts currently on the kernel stack */
static volatile unsigned int intr_depth;

static const char* exception_names[] = {
    "reset stack pointer",
    "reset program counter",
    "bus error",
    "address error",
    "illegal instruction",
    "zero divide",
    "CHK instruction",
    "TRAPV instruction",
    "privilege violation",
    "trace",
    "line 1010 emulator",
    "line 1111 emulator",
    "reserved",
    "reserved",
    "reserved",
    "uninitialized interrupt",
};

void m68k_trap(unsigned int vector, struct iregs* frame) {
	uint8_t*    ef = (uint8_t*)frame + sizeof(struct iregs);
	uint16_t    sr;
	uint32_t    pc;
	uint32_t    address;
	const char* name;
	int	    i;

	if(vector == VECTOR_SPURIOUS) return;

	if(vector >= VECTOR_AUTOVECTOR_1 && vector <= VECTOR_AUTOVECTOR_7) {
		intr_depth++;
		intr_dispatch(vector, frame);
		intr_depth--;

		if(intr_depth == 0 && sched_started) sched_preempt((thread_context_t*)frame);

		return;
	}

	if(vector < sizeof(exception_names) / sizeof(exception_names[0])) {
		name = exception_names[vector];
	} else if(vector >= VECTOR_TRAP_0 && vector <= VECTOR_TRAP_15) {
		name = "TRAP instruction";
	} else {
		name = "unexpected vector";
	}

	if(vector == VECTOR_BUS_ERROR || vector == VECTOR_ADDRESS_ERROR) {
		/* group 0 frame: special status word, access address, instruction register, SR, PC */
		address = *(volatile uint32_t*)(ef + 2);
		sr	= *(volatile uint16_t*)(ef + 8);
		pc	= *(volatile uint32_t*)(ef + 10);
		kprintf("\r\n%s at pc 0x%x, accessing 0x%x\r\n", name, pc, address);
	} else {
		sr = *(volatile uint16_t*)ef;
		pc = *(volatile uint32_t*)(ef + 2);
		kprintf("\r\n%s (vector %d) at pc 0x%x\r\n", name, vector, pc);
	}

	kprintf("sr 0x%x", sr);
	for(i = 0; i < 8; i++) kprintf(" d%d 0x%x", i, frame->d[i]);
	kprintf("\r\n");
	for(i = 0; i < 7; i++) kprintf(" a%d 0x%x", i, frame->a[i]);
	kprintf("\r\n");

	panic("unhandled exception");
}
