	.text

	.globl	_syscall
_syscall:
	moveml	%d2-%d5,%sp@-
	movel	20(%sp),%d0
	movel	24(%sp),%d1
	movel	28(%sp),%d2
	movel	32(%sp),%d3
	movel	36(%sp),%d4
	movel	40(%sp),%d5
	trap	#0
	moveml	%sp@+,%d2-%d5
	rts
