	.text

	.globl	_spinlock_trylock
_spinlock_trylock:
	movel	4(%sp),%a0
	moveq	#0,%d0
	tas	(%a0)
	seq	%d0
	andl	#1,%d0
	rts

	.globl	_spinlock_unlock
_spinlock_unlock:
	movel	4(%sp),%a0
	clrl	(%a0)
	rts
