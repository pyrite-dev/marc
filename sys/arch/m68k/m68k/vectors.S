	.altmacro

	/* thread_context_t / struct iregs layout */
	.set	CTX_SIZE, 72
	.set	CTX_SP, 60
	.set	CTX_SR, 64
	.set	CTX_PC, 68

	.macro SAVE_CONTEXT
	oriw	#0x0700,%sr
	lea	-CTX_SIZE(%sp),%sp
	moveml	%d0-%d7/%a0-%a6,(%sp)
	lea	CTX_SIZE(%sp),%a0
	moveq	#0,%d0
	movew	(%a0),%d0
	movel	%d0,CTX_SR(%sp)
	movel	2(%a0),%d0
	movel	%d0,CTX_PC(%sp)
	lea	6(%a0),%a1
	movel	%a1,CTX_SP(%sp)
	.endm

	.macro RESTORE_CONTEXT
	movel	CTX_SP(%sp),%a0
	moveml	4(%sp),%d1-%d7
	moveml	36(%sp),%a1-%a6
	movel	CTX_PC(%sp),%d0
	movel	%d0,-(%a0)
	movel	CTX_SR(%sp),%d0
	movew	%d0,-(%a0)
	movel	0(%sp),%d0
	movel	%d0,-(%a0)
	movel	32(%sp),%d0
	movel	%d0,-(%a0)
	movel	%a0,%sp
	movel	(%sp),%a0
	movel	4(%sp),%d0
	addql	#8,%sp
	rte
	.endm

	.macro VECTOR_STUB n
m68k_vector_\n:
	SAVE_CONTEXT
	movel	%sp,-(%sp)
	movel	#\n,-(%sp)
	jsr	m68k_trap
	addql	#8,%sp
	RESTORE_CONTEXT
	.endm

	.macro VECTOR_ENTRY n
	.long	m68k_vector_\n
	.endm

	.text

	.set	vec, 2
	.rept	62
	VECTOR_STUB %vec
	.set	vec, vec + 1
	.endr

	.data
	.globl	m68k_vector_stubs
m68k_vector_stubs:
	.long	0, 0
	.set	vec, 2
	.rept	62
	VECTOR_ENTRY %vec
	.set	vec, vec + 1
	.endr

	.text

	.globl	_sched_intr
_sched_intr:
	tstl	sched_started
	beq	1f
	SAVE_CONTEXT
	jsr	sched_tick
	movel	%sp,-(%sp)
	jsr	sched_preempt
	addql	#4,%sp
	RESTORE_CONTEXT
1:
	rte

	.globl	_intr_save
_intr_save:
	clrl	%d0
	movew	%sr,%d0
	oriw	#0x0700,%sr
	rts

	.globl	_intr_restore
_intr_restore:
	movel	4(%sp),%d0
	movew	%d0,%sr
	rts
